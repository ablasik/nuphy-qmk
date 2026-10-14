// The NuPhy Air96 V2 board around the STM32F072: key switches, the two mode switches on the
// back edge (cable/wireless, Mac/Windows) and the LED power path (DC boost on C2, IS31FL3733
// SDB on C6/C7). Pin map from matrix.c and config.h.
using System;
using System.Collections.Generic;
using System.Linq;
using Antmicro.Renode.Core;
using Antmicro.Renode.Exceptions;
using Antmicro.Renode.Peripherals.CPU;
using Antmicro.Renode.Peripherals.Bus;
using Antmicro.Renode.Peripherals.GPIOPort;
using ELFSharp.ELF;
using ELFSharp.ELF.Sections;

namespace Antmicro.Renode.Peripherals.Miscellaneous
{
    public class NuphyAirV2 : IPeripheral
    {
        public NuphyAirV2(IMachine machine, BaseGPIOPort portA, BaseGPIOPort portB, BaseGPIOPort portC, BaseGPIOPort portD,
                          IS31FL3733 led1, IS31FL3733 led2)
        {
            this.machine = machine;
            bus = machine.SystemBus;
            ports = new Dictionary<char, BaseGPIOPort> { { 'A', portA }, { 'B', portB }, { 'C', portC }, { 'D', portD } };
            leds = new[] { led1, led2 };
            inputsOn = ports.Keys.ToDictionary(p => p, p => Cols.Concat(Rows).Concat(new[] { DevMode, SysMode }).Where(pin => pin.Item1 == p).ToArray());

            Output(Boost).AddStateChangedHook(level => boost = level);
            Output(Sdb1).AddStateChangedHook(level => led1.OnGPIO(0, level));
            Output(Sdb2).AddStateChangedHook(level => led2.OnGPIO(0, level));
        }

        public void Reset() { }

        // after the platform is registered: the switches are resolved when the firmware reads a
        // port, like real contacts (Renode's GPIO port rewrites its inputs on every output write,
        // so driving them doesn't stick)
        public void Connect()
        {
            foreach(var port in ports)
            {
                var name = port.Key;
                bus.SetHookAfterPeripheralRead<uint>((IBusPeripheral)port.Value, (value, offset) => offset == IDR ? ReadInputs(name, value) : value);
            }
        }

        // ---- keys, by matrix position (keyboard.json)
        public void Press(int row, int col) => Switch((row, col), true);

        public void Release(int row, int col) => Switch((row, col), false);

        // ---- mode switches: C0 high = cable, C1 high = Mac
        public bool Wireless { get; set; } = true;

        public bool Mac { get; set; } = true;

        // ---- LEDs: what the RGB LED under a key emits, as "r g b"; "0 0 0" while unpowered
        public string KeyLight(int row, int col) => Light(bus.ReadByte(Symbol("g_led_config") + (ulong)(row * Cols.Length + col)));

        public string Light(int index)
        {
            var entry = Symbol("g_is31fl3733_leds") + (ulong)(4 * index);
            var driver = leds[bus.ReadByte(entry) & 3]; // driver : 2
            var rgb = Enumerable.Range(1, 3).Select(i => boost ? driver.Output(bus.ReadByte(entry + (ulong)i)) : 0);
            return string.Join(" ", rgb);
        }

        public bool LedPower => boost;

        // ---- firmware variables by name, for asserting on state
        public void LoadSymbols(string elf)
        {
            // exact values: Renode's own lookup clears bit 0 (a function's Thumb bit), but packed
            // tables like g_led_config really start at odd addresses
            var symtab = (SymbolTable<uint>)ELFReader.Load<uint>(elf).GetSection(".symtab");
            foreach(var entry in symtab.Entries)
            {
                symbols[entry.Name] = entry.Value;
            }
        }

        public byte Byte(string name) => bus.ReadByte(Symbol(name));

        public uint DoubleWord(string name, int index = 0) => bus.ReadDoubleWord(Symbol(name) + (ulong)(4 * index));

        // write one, to fast-forward a timer or counter
        public void SetByte(string name, byte value) => bus.WriteByte(Symbol(name), value);

        public void SetDoubleWord(string name, uint value) => bus.WriteDoubleWord(Symbol(name), value);

        // R0 when the last Call() returned, and whether it has
        public uint CallResult { get; private set; }

        public bool CallDone { get; private set; }

        // ---- firmware calls: run `function` (Thumb, up to four word arguments) from the main loop
        // as if called there: when dev_sts_sync (every housekeeping pass; LTO inlined the rest of
        // housekeeping_task_kb) next starts, it calls the function first,
        // then carries on with every register as it was. Stands in for host commands over USB
        // (VIA), which isn't emulated.
        public void Call(string function, uint a0 = 0, uint a1 = 0, uint a2 = 0, uint a3 = 0)
        {
            var cpu = machine.SystemBus.GetCPUs().OfType<CortexM>().First();
            var entry = Symbol("dev_sts_sync") & ~1ul;
            var target = Symbol(function) & ~1ul;
            var saved = (ulong[])null;
            CallDone = false;
            CpuAddressHook hook = null;
            hook = (c, pc) =>
            {
                if(saved == null)
                {
                    saved = Enumerable.Range(0, 15).Select(i => cpu.GetRegister(i).RawValue).ToArray();
                    var args = new[] { a0, a1, a2, a3 };
                    for(var i = 0; i < args.Length; i++) { cpu.SetRegister(i, args[i]); }
                    cpu.SetRegister(LR, entry | 1); // return here, Thumb
                    cpu.PC = target;
                    return;
                }
                CallResult = (uint)cpu.GetRegister(0).RawValue;
                CallDone = true;
                for(var i = 0; i < saved.Length; i++) { cpu.SetRegister(i, saved[i]); } // R0-R12, SP, LR
                cpu.RemoveHook(entry, hook);
            };
            cpu.AddHook(entry, hook);
        }

        // a different QMK_BUILDDATE in the loaded firmware, as a new build of the same code would
        // have: QMK keys some EEPROM contents (VIA's keymap) to it
        public void NewBuildDate()
        {
            var flash = bus.ReadBytes(0x08000000, 0x20000);
            var text = System.Text.Encoding.ASCII.GetString(flash);
            var date = System.Text.RegularExpressions.Regex.Match(text, @"20\d\d-\d\d-\d\d-\d\d:\d\d:\d\d");
            if(!date.Success) { throw new RecoverableException("No QMK_BUILDDATE in flash"); }
            var other = date.Value == "2000-01-01-00:00:00" ? "2000-01-01-00:00:01" : "2000-01-01-00:00:00";
            bus.WriteBytes(System.Text.Encoding.ASCII.GetBytes(other), 0x08000000ul + (ulong)date.Index);
        }

        private void Switch((int, int) rc, bool down)
        {
            var row = Rows[rc.Item1];
            var before = Level(row);
            if(down) { pressed.Add(rc); } else { pressed.Remove(rc); }
            // in deep sleep (mcu_pwr.c) the rows are pulled-down inputs and the columns driven high:
            // closing a switch raises its row, an edge (EXTI) that wakes the MCU. Renode's pin still
            // holds the level it last drove as an output, so set where the pull left it first.
            if(IsInput(row))
            {
                ports[row.Item1].OnGPIO(row.Item2, before);
                ports[row.Item1].OnGPIO(row.Item2, Level(row));
            }
        }

        // called for every IDR read (the scan reads thousands a second): keep it cheap
        private uint ReadInputs(char port, uint idr)
        {
            var moder = Register(port, MODER);
            foreach(var pin in inputsOn[port])
            {
                if(((moder >> (2 * pin.Item2)) & 3) != 0) { continue; } // not an input now
                var level = pressed.Count == 0 ? IdleLevel(pin) : Level(pin);
                idr = level ? idr | (1u << pin.Item2) : idr & ~(1u << pin.Item2);
            }
            return idr;
        }

        // nothing pressed: columns pulled up, rows pulled down, mode switches where they're set
        private bool IdleLevel((char, int) pin) => pin == DevMode ? !Wireless : pin == SysMode ? Mac : Array.IndexOf(Rows, pin) < 0;

        // the level an input pin sees through the switches
        private bool Level((char, int) pin)
        {
            if(pin == DevMode) { return !Wireless; }
            if(pin == SysMode) { return Mac; }
            var c = Array.IndexOf(Cols, pin);
            if(c >= 0) // pulled up; a closed switch to a row driven low pulls it down
            {
                return !pressed.Any(k => k.Item2 == c && Driven(Rows[k.Item1]) == false);
            }
            var r = Array.IndexOf(Rows, pin); // pulled down; a closed switch to a column driven high raises it
            return pressed.Any(k => k.Item1 == r && Driven(Cols[k.Item2]) == true);
        }

        // what an output pin drives, or null for an input
        private bool? Driven((char, int) pin)
        {
            if(IsInput(pin)) { return null; }
            return ((Register(pin.Item1, ODR) >> pin.Item2) & 1) != 0;
        }

        private bool IsInput((char, int) pin) => ((Register(pin.Item1, MODER) >> (2 * pin.Item2)) & 3) == 0;

        private uint Register(char port, long offset) => ((IDoubleWordPeripheral)ports[port]).ReadDoubleWord(offset);

        private GPIO Output((char, int) pin) => (GPIO)ports[pin.Item1].Connections[pin.Item2];

        // a function's static is named like `last.2` after LTO: the plain name finds it if unique
        private ulong Symbol(string name)
        {
            if(symbols.TryGetValue(name, out var address)) { return address; }
            var statics = symbols.Where(s => s.Key.StartsWith(name + ".") && s.Key.Substring(name.Length + 1).All(char.IsDigit)).ToArray();
            if(statics.Length != 1) { throw new RecoverableException($"No single symbol {name}: {string.Join(", ", statics.Select(s => s.Key))}"); }
            return statics[0].Value;
        }

        private readonly IMachine machine;
        private readonly IBusController bus;
        private readonly Dictionary<char, BaseGPIOPort> ports;
        private readonly IS31FL3733[] leds;
        private readonly Dictionary<char, (char, int)[]> inputsOn; // the switch pins on each port
        private readonly HashSet<(int, int)> pressed = new HashSet<(int, int)>();
        private readonly Dictionary<string, ulong> symbols = new Dictionary<string, ulong>();
        private bool boost;

        private const long MODER = 0x00, IDR = 0x10, ODR = 0x14;
        private const int LR = 14;

        private static readonly (char, int)[] Rows = { ('C', 14), ('C', 15), ('A', 0), ('A', 1), ('A', 2), ('A', 3) };
        private static readonly (char, int)[] Cols =
        {
            ('A', 4), ('A', 5), ('A', 6), ('A', 7), ('B', 0), ('B', 1), ('B', 10), ('B', 11), ('B', 12), ('B', 13), ('B', 14),
            ('B', 15), ('A', 8), ('A', 9), ('A', 10), ('A', 15), ('B', 3), ('C', 10), ('C', 11), ('C', 12), ('D', 2),
        };
        private static readonly (char, int) DevMode = ('C', 0), SysMode = ('C', 1), Boost = ('C', 2), Sdb1 = ('C', 6), Sdb2 = ('C', 7);
    }
}
