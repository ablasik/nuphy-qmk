// The RF module (BT/2.4G chip) on USART1, from the keyboard's side of the protocol in rf.c:
// frames are 5A cmd flag len payload[len] checksum; an ACK is 5A cmd A0.
// It answers like a module connected to a host over BT 1, on battery; the properties below
// change that, and SET_LINK moves it to another slot.
using System.Collections.Generic;
using System.Linq;
using Antmicro.Renode.Peripherals.UART;

namespace Antmicro.Renode.Peripherals.Miscellaneous
{
    public class NuphyRf : IPeripheral
    {
        public NuphyRf(IUART uart)
        {
            this.uart = uart;
            uart.CharReceived += OnChar;
        }

        public void Reset()
        {
            rx.Clear();
            frames.Clear();
        }

        public byte Link { get; set; } = 1;     // LINK_BT_1
        public byte State { get; set; } = 3;    // RF_CONNECT
        public byte Charge { get; set; } = 0;   // on battery (0x03 = cable in: USB_ACTIVE)
        public byte Battery { get; set; } = 80;
        public byte Leds { get; set; } = 0;     // the host's keyboard LEDs: bit 0 Num Lock, bit 1 Caps Lock

        // frames of `cmd` the keyboard sent so far
        public int Count(byte cmd) => frames.Count(f => f.Item1 == cmd);

        // payload of the last `cmd` frame, as hex bytes, or "" if none
        public string Last(byte cmd) =>
            string.Join(" ", frames.LastOrDefault(f => f.Item1 == cmd).Item2?.Select(b => b.ToString("X2")) ?? new string[0]);

        // every frame since the last Clear(), "cmd: payload" per line
        public string Log() => string.Join("\n", frames.Select(f => f.Item1.ToString("X2") + ": " + string.Join(" ", f.Item2.Select(b => b.ToString("X2")))));

        public void Clear() => frames.Clear();

        // the last bytes received, raw, for diagnosing framing
        public string Raw => string.Join(" ", raw.Select(b => b.ToString("X2")));

        private void OnChar(byte b)
        {
            raw.Enqueue(b);
            while(raw.Count > 96) { raw.Dequeue(); }
            if(rx.Count == 0 && b != Head)
            {
                return;
            }
            rx.Add(b);
            if(rx.Count >= 4 && rx.Count == rx[3] + 5)
            {
                var cmd = rx[1];
                var payload = rx.Skip(4).Take(rx[3]).ToArray();
                rx.Clear();
                frames.Add((cmd, payload));
                Answer(cmd);
            }
        }

        private void Answer(byte cmd)
        {
            switch(cmd)
            {
            case 0xF2: // CMD_HAND
                Reply(cmd);
                break;
            case 0x81: // CMD_READ_DATA: link mode, rf channel, ble channel at [4..6]
                Reply(cmd, new byte[] { 0, 0, 0, 0, Link, Link, Link }.Concat(new byte[25]).ToArray());
                break;
            case 0xC9: // CMD_RF_STS_SYSC
                Reply(cmd, Link, State, Leds, Charge, Battery);
                break;
            case 0xC0: // CMD_SET_LINK: switch to the slot asked for (and connect right away)
                Link = frames[frames.Count - 1].Item2[0];
                Send(Head, cmd, 0xA0);
                break;
            default:
                if(cmd < 0xE0 || cmd > 0xE4) // reports aren't acknowledged
                {
                    Send(Head, cmd, 0xA0);
                }
                break;
            }
        }

        private void Reply(byte cmd, params byte[] payload)
        {
            Send(new byte[] { Head, cmd, 0, (byte)payload.Length }.Concat(payload).Concat(new[] { (byte)payload.Sum(b => b) }).ToArray());
        }

        private void Send(params byte[] data)
        {
            foreach(var b in data)
            {
                uart.WriteChar(b);
            }
        }

        private readonly IUART uart;
        private readonly List<byte> rx = new List<byte>();
        private readonly Queue<byte> raw = new Queue<byte>();
        private readonly List<(byte, byte[])> frames = new List<(byte, byte[])>();

        private const byte Head = 0x5A;
    }
}
