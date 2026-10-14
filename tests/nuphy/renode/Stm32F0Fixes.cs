// Stand-ins for the parts of Renode's STM32F0 platform that stop QMK/ChibiOS from booting.
// Each models the real register behaviour the firmware relies on, nothing more.
using System.Collections.Generic;
using System.Linq;
using Antmicro.Renode.Core;
using Antmicro.Renode.Logging;
using Antmicro.Renode.Peripherals.Bus;
using Antmicro.Renode.Peripherals.CPU;

namespace Antmicro.Renode.Peripherals.Miscellaneous
{
    // RCC: Renode's is a flip-flop script that never reports LSI ready. Registers keep what's
    // written; every oscillator is ready and the clock switch takes effect at once.
    [AllowedTranslations(AllowedTranslation.ByteToDoubleWord | AllowedTranslation.WordToDoubleWord)]
    public class Stm32F0Rcc : IDoubleWordPeripheral, IKnownSize
    {
        public long Size => 0x400;

        public void Reset() => regs.Clear();

        public void WriteDoubleWord(long offset, uint value) => regs[offset] = value;

        public uint ReadDoubleWord(long offset)
        {
            regs.TryGetValue(offset, out var v);
            switch(offset)
            {
            case 0x00: return v | (1u << 1) | (1u << 17) | (1u << 25);  // CR: HSIRDY HSERDY PLLRDY
            case 0x04: return (v & ~0xCu) | ((v & 3) << 2);            // CFGR: SWS = SW
            case 0x20: return v | (1u << 1);                           // BDCR: LSERDY
            case 0x24: return v | (1u << 1);                           // CSR: LSIRDY
            case 0x34: return v | (1u << 1) | (1u << 17);              // CR2: HSI14RDY HSI48RDY
            default: return v;
            }
        }

        private readonly Dictionary<long, uint> regs = new Dictionary<long, uint>();
    }

    // FLASH interface (not in Renode's platform): ACR reads back, never busy, status flags clear
    // by writing 1, page erase fills the 2 KB page with 0xFF. Programming writes go straight to
    // the flash memory.
    [AllowedTranslations(AllowedTranslation.ByteToDoubleWord | AllowedTranslation.WordToDoubleWord)]
    public class Stm32F0FlashInterface : IDoubleWordPeripheral, IKnownSize
    {
        public Stm32F0FlashInterface(IMachine machine)
        {
            bus = machine.GetSystemBus(this);
        }

        public long Size => 0x400;

        public void Reset() => regs.Clear();

        // a factory-fresh chip: everything erased (call before loading the firmware)
        public void EraseAll()
        {
            for(ulong a = FlashStart; a < FlashStart + FlashSize; a += 4)
            {
                bus.WriteDoubleWord(a, 0xFFFFFFFF);
            }
        }

        public void WriteDoubleWord(long offset, uint value)
        {
            if(offset == SR) // status flags clear by writing 1
            {
                regs.TryGetValue(SR, out var flags);
                regs[SR] = flags & ~value;
                return;
            }
            regs[offset] = value;
            if(offset == CR && (value & Strt) != 0)
            {
                if((value & Per) != 0)
                {
                    regs.TryGetValue(AR, out var ar);
                    var page = (ulong)ar & ~0x7FFul;
                    this.Log(LogLevel.Debug, "Erasing page 0x{0:X}", page);
                    for(var a = page; a < page + 0x800; a += 4)
                    {
                        bus.WriteDoubleWord(a, 0xFFFFFFFF);
                    }
                }
                regs[CR] = value & ~Strt;
                regs.TryGetValue(SR, out var sr);
                regs[SR] = sr | Eop;
            }
        }

        public uint ReadDoubleWord(long offset)
        {
            regs.TryGetValue(offset, out var v);
            return offset == SR ? v & ~1u : v; // never BSY
        }

        private readonly IBusController bus;
        private readonly Dictionary<long, uint> regs = new Dictionary<long, uint>();

        private const long SR = 0x0C, CR = 0x10, AR = 0x14;
        private const uint Per = 1u << 1, Strt = 1u << 6, Eop = 1u << 5;
        private const ulong FlashStart = 0x08000000, FlashSize = 0x20000;
    }

    // TIM14 runs wait_us() as a polled one-pulse delay (gptPolledDelay). Renode's timer ends
    // the pulse but never raises UIF, so count the pulse out in executed instructions
    // (virtual time only moves per quantum, far too coarse for microseconds).
    [AllowedTranslations(AllowedTranslation.ByteToDoubleWord | AllowedTranslation.WordToDoubleWord)]
    public class Stm32OnePulseTimer : IDoubleWordPeripheral, IKnownSize
    {
        public Stm32OnePulseTimer(IMachine machine, long frequency)
        {
            this.machine = machine;
            this.frequency = frequency;
        }

        public long Size => 0x400;

        public void Reset() => regs.Clear();

        public void WriteDoubleWord(long offset, uint value)
        {
            regs[offset] = value;
            if(offset == CR1 && (value & Cen) != 0)
            {
                start = Cpu.ExecutedInstructions;
            }
        }

        public uint ReadDoubleWord(long offset)
        {
            if(offset == SR && (Reg(CR1) & Cen) != 0)
            {
                var ticks = (ulong)(Reg(PSC) + 1) * (Reg(ARR) + 1);
                var elapsedUs = (Cpu.ExecutedInstructions - start) / Cpu.PerformanceInMips;
                if(elapsedUs * (ulong)frequency / 1000000 >= ticks)
                {
                    regs[SR] = Reg(SR) | Uif;
                    regs[CR1] = Reg(CR1) & ~Cen; // one pulse
                }
            }
            return Reg(offset);
        }

        private uint Reg(long offset)
        {
            regs.TryGetValue(offset, out var v);
            return v;
        }

        private TranslationCPU Cpu => cpu ?? (cpu = machine.SystemBus.GetCPUs().OfType<TranslationCPU>().First());

        private readonly IMachine machine;
        private readonly long frequency;
        private readonly Dictionary<long, uint> regs = new Dictionary<long, uint>();
        private TranslationCPU cpu;
        private ulong start;

        private const long CR1 = 0x00, SR = 0x10, PSC = 0x28, ARR = 0x2C;
        private const uint Cen = 1, Uif = 1;
    }

    // ChibiOS on ARMv6-M leaves an ISR by pending NMI with interrupts masked and spinning until
    // it fires (__port_exit_from_isr). A real NMI ignores PRIMASK; Renode's doesn't, so unmask
    // at the spin (the NMI handler unmasks right after anyway). Arm() after loading the ELF.
    public class ChibiosNmiExit : IPeripheral
    {
        public ChibiosNmiExit(IMachine machine)
        {
            this.machine = machine;
        }

        public void Reset() { }

        public void Arm()
        {
            var cpu = machine.SystemBus.GetCPUs().OfType<CortexM>().First();
            var spin = machine.SystemBus.GetSymbolAddress("__port_exit_from_isr") + 6;
            cpu.AddHook(spin, (c, addr) => cpu.SetRegisterUlong("PRIMASK", 0));
        }

        private readonly IMachine machine;
    }
}
