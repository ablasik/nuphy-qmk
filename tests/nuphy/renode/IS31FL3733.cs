// IS31FL3733 12x16 dot matrix LED driver (ISSI datasheet rev. E), as far as a keyboard uses it:
// write-locked command register, four pages, auto-incrementing register address,
// software shutdown, global current, reset-on-read and the SDB hardware shutdown pin.
// Open/short detection and auto breath timing aren't modelled; their registers just hold values.
using System;
using Antmicro.Renode.Core;
using Antmicro.Renode.Logging;
using Antmicro.Renode.Peripherals.I2C;

namespace Antmicro.Renode.Peripherals.Miscellaneous
{
    public class IS31FL3733 : II2CPeripheral, IGPIOReceiver
    {
        public IS31FL3733()
        {
            Reset();
        }

        public void Reset()
        {
            Array.Clear(ledControl, 0, ledControl.Length);
            Array.Clear(pwm, 0, pwm.Length);
            Array.Clear(breath, 0, breath.Length);
            Array.Clear(function, 0, function.Length);
            page = 0;
            unlocked = false;
            pointer = null;
        }

        // SDB: low = hardware shutdown (registers keep their values)
        public void OnGPIO(int number, bool value)
        {
            Sdb = value;
        }

        public void Write(byte[] data)
        {
            this.Log(LogLevel.Noisy, "Write {0}", BitConverter.ToString(data));
            foreach(var b in data)
            {
                if(pointer == null)
                {
                    pointer = b;
                    continue;
                }
                WriteRegister(pointer.Value, b);
                pointer = (byte)(pointer.Value + 1);
            }
        }

        public byte[] Read(int count = 1)
        {
            var result = new byte[count];
            for(var i = 0; i < count; i++)
            {
                result[i] = ReadRegister(pointer ?? 0);
                pointer = (byte)((pointer ?? 0) + 1);
            }
            return result;
        }

        public void FinishTransmission()
        {
            this.Log(LogLevel.Noisy, "Stop");
            pointer = null;
        }

        public bool Sdb { get; set; }

        public bool SoftwareShutdown => (function[0x00] & 0x01) == 0;

        public byte GlobalCurrent => function[0x01];

        public bool LedEnabled(int index) => (ledControl[index / 8] & (1 << (index % 8))) != 0;

        public byte Pwm(int index) => pwm[index];

        // what the LED at SWy/CSx (index = (y-1)*16 + x-1) actually gets: PWM scaled by the
        // global current, nothing while shut down or switched off
        public int Output(int index)
        {
            if(!Sdb || SoftwareShutdown || !LedEnabled(index))
            {
                return 0;
            }
            return pwm[index] * GlobalCurrent / 255;
        }

        private void WriteRegister(byte reg, byte value)
        {
            if(reg == CommandWriteLock)
            {
                unlocked = value == WriteLockMagic;
                return;
            }
            if(reg == Command)
            {
                if(!unlocked)
                {
                    this.Log(LogLevel.Warning, "Page select 0x{0:X} ignored: command register is locked", value);
                    return;
                }
                unlocked = false; // the lock re-arms after one command write
                if(value > 3)
                {
                    this.Log(LogLevel.Warning, "No page 0x{0:X}", value);
                    return;
                }
                page = value;
                return;
            }
            if(reg == InterruptMask || reg == InterruptStatus)
            {
                return;
            }
            switch(page)
            {
            case 0:
                if(reg < ledControl.Length)
                {
                    ledControl[reg] = value;
                } // 0x18-0x47: open/short status, read only
                break;
            case 1:
                if(reg < pwm.Length) { pwm[reg] = value; }
                break;
            case 2:
                if(reg < breath.Length) { breath[reg] = value; }
                break;
            case 3:
                if(reg < function.Length && reg != 0x11) { function[reg] = value; }
                break;
            }
        }

        private byte ReadRegister(byte reg)
        {
            switch(page)
            {
            case 0:
                return reg < ledControl.Length ? ledControl[reg] : (byte)0; // no opens, no shorts
            case 1:
                return reg < pwm.Length ? pwm[reg] : (byte)0;
            case 2:
                return reg < breath.Length ? breath[reg] : (byte)0;
            default:
                if(reg == 0x11)
                {
                    var keepPage = page;
                    Reset(); // reading the reset register returns every register to its default
                    page = keepPage;
                    return 0;
                }
                return reg < function.Length ? function[reg] : (byte)0;
            }
        }

        private readonly byte[] ledControl = new byte[0x18];
        private readonly byte[] pwm = new byte[0xC0];
        private readonly byte[] breath = new byte[0xC0];
        private readonly byte[] function = new byte[0x12];
        private byte page;
        private bool unlocked;
        private byte? pointer;

        private const byte Command = 0xFD;
        private const byte CommandWriteLock = 0xFE;
        private const byte WriteLockMagic = 0xC5;
        private const byte InterruptMask = 0xF0;
        private const byte InterruptStatus = 0xF1;
    }
}
