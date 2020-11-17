
using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Net;
using System.Net.NetworkInformation;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Timers;
using System.Windows.Forms;

namespace CANguruX
{
    public partial class Form1 : Form
    {
        enum CMD
        {
            toCAN, toClnt, toWDP, fromWDP, fromWDP2CAN, fromCAN2WDP, fromClnt, toGW, fromGW
        };
        private TcpClient Client = null;
        Thread connectThread;
        NetworkStream stream;
        StreamReader reader;
        private readonly ConcurrentQueue<String> _queue = new ConcurrentQueue<String>();

        // This delegate enables asynchronous calls for setting
        // the text property on a TextBox control.
        // Delegates
        //
        delegate void UpdateProgressPingBarDelegate();
        UpdateProgressPingBarDelegate UpdateProgressPingBarMethod;
        //
        delegate void UpdateProgressMFXBarDelegate();
        UpdateProgressMFXBarDelegate UpdateProgressMFXBarMethod;
        //
        delegate String GetMyTextDelegate(Control ctrl);
        delegate void ChangeMyTextDelegate(TextBox ctrl, string text);

        // buffers for receiving and sending data
        byte[] GETCONFIG_RESPONSE = { 0x00, 0x42, 0x03, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
        //
        IniFile ini = new IniFile();

        Thread threadCAN;
        // UDP to CAN
        public UdpClient CANServer;
        public UdpClient CANClient;
        //
        bool Voltage;
        CQueue myQueue;
        Cnames names;
        CConfigStream ConfigStream;
        //
        private static System.Timers.Timer a1secTimer;
        byte elapsedsec;
        //
        private static System.Timers.Timer a1milliTimer;
        Int16 elapsedmillis;
        bool receivePINGInfos;
        byte[,] CANguruArr;
        byte[,] CANguruPINGArr;
        struct configStruct
        {
            public Label indicatorLabel;
            public NumericUpDown indicatorNumericUpDown;
            public ListBox indicatorListBox;
            public int minValue;
            public int maxValue;
            public int currValue;
            public Label unit;
        }
        configStruct[] CANguruConfigArr;
        struct structConfigControls
        {
            public byte cntControls;
            public Control[] controlLabel;
            public Control[] controlUnit;
            public Control[] controlValue;
            public Control[] controlChoice;
        }
        structConfigControls configControls;
        byte CANguruArrFilled;
        byte CANguruArrIndex;
        byte CANguruArrWorked;
        byte CANguruArrLine;
        byte lastSelectedItem;
        const byte DEVTYPE_BASE = 0x50;
        const byte DEVTYPE_SERVO = 0x53;
        const byte DEVTYPE_RM = 0x54;
        const byte DEVTYPE_LIGHT = 0x55;
        const byte DEVTYPE_SIGNAL = 0x56;
        const byte DEVTYPE_LEDSIGNAL = 0x57;
        const byte DEVTYPE_CANFUSE = 0x58;
        const byte DEVTYPE_LastCANguru = 0x5F;
        //
        List<byte[]> WeichenList = new List<byte[]>();
        //
        bool is_connected;

        public Form1()
        {
            InitializeComponent();
            Voltage = false;
            is_connected = false;
            CANguruArr = new byte[Cnames.maxConfigLines, Cnames.lngFrame]; // [maxConfigLines+1][Cnames.lngFrame]
            CANguruPINGArr = new byte[Cnames.maxCANgurus, Cnames.lngFrame + 1]; // [maxCANgurus][maxConfigLines+1][Cnames.lngFrame]
                                                                                //
                                                                                // (common section's keys will overwrite)
                                                                                // initialize the delegate here
            UpdateProgressPingBarMethod = new UpdateProgressPingBarDelegate(UpdateProgressPingBar);
            UpdateProgressMFXBarMethod = new UpdateProgressMFXBarDelegate(UpdateProgressMFXBar);
            this.progressBarPing.Visible = false;
            this.progressBarMfx.Visible = false;
            try
            {
                myQueue = new CQueue();
                names = new Cnames();
                // CANServer empfängt von CAN (Arduino-Gateway)
                CANServer = new UdpClient(Cnames.portinCAN);
                // CANClient sendet nach UDP (WDP)
                CANClient = new UdpClient();
                ConfigStream = new CConfigStream(names);
                // Determine whether the directory exists.
                string iniStr = string.Concat(Cnames.path, Cnames.ininame);
                bool fexists = File.Exists(iniStr);
                //                if (Directory.Exists(Cnames.path))
                if (fexists)
                {
                    ini.Load(iniStr);
                    //  Returns a KeyValue in a certain section
                    Cnames.IP_CAN = ini.GetKeyValue("IP-address", "IPCAN");
                    byte c = 0;
                    if (!byte.TryParse(ini.GetKeyValue("Neuanmeldezaehler", "Counter"), out c))
                        c = 2;
                    if (c < 2)
                        c = 2;
                    ConfigStream.setCounter(c);
                    byte n = 0;
                    if (!byte.TryParse(ini.GetKeyValue("Lok-Adresse", "LocID"), out n))
                        n = 5;
                    if (n < 5)
                        n = 5;
                    ConfigStream.setnextLocid(n);
                }
                else
                {
                    // Try to create the directory.
                    DirectoryInfo di = Directory.CreateDirectory(Cnames.path);
                    ini.AddSection("IP-address").AddKey("IPCAN").Value = Cnames.IP_CAN;
                    ini.AddSection("Neuanmeldezaehler").AddKey("Counter").Value = "2";
                    ini.AddSection("Lok-Adresse").AddKey("LocID").Value = "5";
                }
                txtbHost_CAN.Text = Cnames.IP_CAN;
                this.Load += Form1_Load;
            }
            catch (Exception e)
            {
                MessageBox.Show("InitializeException: " + e.Message);
            }
        }

        string getindicatorName(ref byte line, ref byte lfd)
        {
            String str = "";
            while (true)
            {
                // in der Zeile 2 steht der Name des Gerätes
                char ch = (char)CANguruArr[line, lfd + 5];
                if (ch == 0x00)
                {
                    lfd++;
                    if (lfd == 8)
                    {
                        lfd = 0;
                        line++;
                    }
                    return str;
                }
                str += ch;
                lfd++;
                if (lfd == 8)
                {
                    lfd = 0;
                    line++;
                }
            }
        }

        private void Form1_Load(object sender, System.EventArgs e)
        {
            // Set the Minimum, Maximum, and initial Value.
            numCounter.Value = ConfigStream.getCounter();
            numCounter.Maximum = 99;
            numCounter.Minimum = 2;
            // Set the Minimum, Maximum, and initial Value.
            numLocID.Value = ConfigStream.getnextLocid();
            numLocID.Maximum = 99;
            numLocID.Minimum = 5;
            //
            // Set the Minimum, Maximum, and initial Value.
            numUpDnDecNumber.Maximum = 255;
            numUpDnDecNumber.Minimum = 1;
            numUpDnDecNumber.Value = 1;
            //
            // Set the Minimum, Maximum, and initial Value.
            numUpDnDelay.Maximum = 15;
            numUpDnDelay.Minimum = 1;
            numUpDnDelay.Value = 10;
            //
            // CAN
            threadCAN = new Thread(new ThreadStart(fromCAN2UDP));
            threadCAN.IsBackground = true;
            threadCAN.Start();
        }

        int getIndicatorValue(byte pos)
        {
            return CANguruArr[0, pos + 5];
        }

        void read1ConfigChannel_DescriptionBlock(ref byte CgArrIndex, ref byte[] content)
        {
            if (CANguruArrWorked < CANguruArrFilled)
            {
                // letzte Zeile eingelesen
                // auswerten des Paketes
                // Eintrag für Listbox erzeugen
                byte line = 2;
                byte pos = 0;
                String entry = getindicatorName(ref line, ref pos);
                // Anzahl der folgenden Messwerte aus Zeile 2, Position 6
                CANguruPINGArr[CANguruArrWorked, Cnames.lngFrame] = CANguruArr[0, 0x06];
                // der Hashwert zur Unterscheidung aus der
                // aktuellen Zeile Position 2 und 3
                entry += "-" + String.Format("{0:X02}", content[0x02]);
                entry += String.Format("{0:X02}", content[0x03]);
                // in Listbox eintragen
                this.CANElemente.Invoke(new MethodInvoker(() => CANElemente.Items.Add(entry)));
                // ersten Messwert vorbereiten
                CANguruArrLine = 0;
                CANguruArrWorked++;
                getConfigData(CANguruArrWorked, CgArrIndex);
                return;
            }
        }

        void readChoiceBlock(ref byte CgArrIndex)
        {
            int numberofLEDProgs = getIndicatorValue(2);
            // > 0 sind Messwertbeschreibungen
            byte maxIndex = CANguruPINGArr[CANguruArrWorked, Cnames.lngFrame];
            int xpos0 = 50;
            int xpos1 = 250;
            int yposDelta = 0;
            for (byte cc = 0; cc < CgArrIndex - 1; cc++)
            {
                if (CANguruConfigArr[cc].indicatorNumericUpDown != null)
                    yposDelta += 25;
                else
                    yposDelta += 65;
            }

            int ypos = 150 + yposDelta;
            byte line = 1;
            byte pos = 0;
            // Wertename
            Label cntrlLabel = new Label();
            CANguruConfigArr[CgArrIndex - 1].indicatorLabel = cntrlLabel;
            cntrlLabel.Text = getindicatorName(ref line, ref pos);
            cntrlLabel.Location = new Point(xpos0, ypos);
            ListBox choiceBox = new ListBox();
            CANguruConfigArr[CgArrIndex - 1].indicatorListBox = choiceBox;
            CANguruConfigArr[CgArrIndex - 1].indicatorNumericUpDown = null;
            for (int ch = 0; ch < numberofLEDProgs; ch++)
            {
                String choice = getindicatorName(ref line, ref pos);
                choiceBox.Items.Add(choice);
            }
            choiceBox.Location = new Point(xpos1, ypos);
            choiceBox.Height = 60;
            // Label anzeigen
            this.Configuration.Invoke(new MethodInvoker(() => Configuration.Controls.Add(cntrlLabel)));
            configControls.controlLabel[CgArrIndex - 1] = cntrlLabel;
            // choice anzeigen
            this.Configuration.Invoke(new MethodInvoker(() => Configuration.Controls.Add(choiceBox)));
            configControls.controlChoice[CgArrIndex - 1] = choiceBox;
            // auswählen
            int select = getIndicatorValue(3);
            this.Configuration.Invoke(new MethodInvoker(() => choiceBox.SetSelected(select, true)));
            CANguruArrLine = 0;
            if (CgArrIndex < maxIndex)
            // letzte Zeile eingelesen
            {
                CgArrIndex++;
                getConfigData(CANguruArrWorked, CgArrIndex);
                return;
            }
            if (CgArrIndex == maxIndex)
            {
                // letzter Messwert eingelesen
                CgArrIndex = 0;
                lastSelectedItem = CANguruArrWorked;
                return;
            }
        }
        void readValueBlock(ref byte CgArrIndex)
        {
            // > 0 sind Messwertbeschreibungen
            byte maxIndex = CANguruPINGArr[CANguruArrWorked, Cnames.lngFrame];
            int xpos0 = 50;
            int xpos1 = 250;
            int ypos = 120 + CgArrIndex * 25;
            int w = 50;
            byte line = 1;
            byte pos = 0;
            // Wertename
            Label cntrlLabel = new Label();
            CANguruConfigArr[CgArrIndex - 1].indicatorLabel = cntrlLabel;
            cntrlLabel.Text = getindicatorName(ref line, ref pos);
            cntrlLabel.Location = new Point(xpos0, ypos);
            // Anfangsbezeichnung
            String notused = getindicatorName(ref line, ref pos);
            // Endebezeichnung
            notused = getindicatorName(ref line, ref pos);
            // Einheit
            Label unitLabel = new Label();
            CANguruConfigArr[CgArrIndex - 1].unit = unitLabel;
            unitLabel.Text = getindicatorName(ref line, ref pos);
            unitLabel.Location = new Point(xpos1 + w + 10, ypos);
            // Wert
            NumericUpDown ctrlNumericUpDown = new NumericUpDown();
            ctrlNumericUpDown.Width = w;
            CANguruConfigArr[CgArrIndex - 1].indicatorNumericUpDown = ctrlNumericUpDown;
            CANguruConfigArr[CgArrIndex - 1].indicatorListBox = null;
            ctrlNumericUpDown.Location = new Point(xpos1, ypos);
            // values big endian
            CANguruConfigArr[CgArrIndex - 1].minValue = (getIndicatorValue(2) << 8) + getIndicatorValue(3);
            ctrlNumericUpDown.Minimum = CANguruConfigArr[CgArrIndex - 1].minValue;
            CANguruConfigArr[CgArrIndex - 1].maxValue = (getIndicatorValue(4) << 8) + getIndicatorValue(5);
            ctrlNumericUpDown.Maximum = CANguruConfigArr[CgArrIndex - 1].maxValue;
            CANguruConfigArr[CgArrIndex - 1].currValue = (getIndicatorValue(6) << 8) + getIndicatorValue(7);
            ctrlNumericUpDown.Value = CANguruConfigArr[CgArrIndex - 1].currValue;
            // Label anzeigen
            this.Configuration.Invoke(new MethodInvoker(() => Configuration.Controls.Add(cntrlLabel)));
            configControls.controlLabel[CgArrIndex - 1] = cntrlLabel;
            // Einheit anzeigen
            this.Configuration.Invoke(new MethodInvoker(() => Configuration.Controls.Add(unitLabel)));
            configControls.controlUnit[CgArrIndex - 1] = unitLabel;
            // Wert anzeigen
            this.Configuration.Invoke(new MethodInvoker(() => Configuration.Controls.Add(ctrlNumericUpDown)));
            configControls.controlValue[CgArrIndex - 1] = ctrlNumericUpDown;
            CANguruArrLine = 0;
            if (CgArrIndex < maxIndex)
            // letzte Zeile eingelesen
            {
                CgArrIndex++;
                getConfigData(CANguruArrWorked, CgArrIndex);
                return;
            }
            if (CgArrIndex == maxIndex)
            {
                // letzter Messwert eingelesen
                CgArrIndex = 0;
                lastSelectedItem = CANguruArrWorked;
                return;
            }
        }
        void read1ConfigChannel_ValueBlock(ref byte CgArrIndex)
        {
            int ptr = getIndicatorValue(1);
            if (ptr == 1)
                readChoiceBlock(ref CgArrIndex);
            else
                readValueBlock(ref CgArrIndex);
        }
        string doMsg4TctWindow(CMD src, byte[] content)
        {
            byte[] msg = { 0x26, 0x30,
            0x24, 0x30,
            0x24, 0x30,
            0x24, 0x30,
            0x24, 0x30,
            0x24, 0x30,
            0x24, 0x30,
            0x24, 0x30,
            0x24, 0x30,
            0x24, 0x30,
            0x24, 0x30,
            0x24, 0x30,
            0x24, 0x30,
            0x24, 0x30,
            };
            if (src == CMD.fromGW)
                msg[0x01] = 0x33;
            if (src == CMD.toGW)
                msg[0x01] = 0x31;
            byte index;
            for (byte ch = 0; ch < content.Length; ch++)
            {
                index = (byte)(2 * ch + 2);
                if (content[ch] < 0x20)
                {
                    msg[index] = (byte)'%';
                    msg[index + 1] = (byte)(content[ch] + '0');
                }
                else
                {
                    if (content[ch] >= 0x80)
                    {
                        msg[index] = (byte)'&';
                        msg[index + 1] = (byte)(content[ch] - 0x80);
                    }
                    else
                    {
                        msg[index] = (byte)'$';
                        msg[index + 1] = content[ch];
                    }
                }
            }
            // From byte array to string
            return System.Text.Encoding.UTF8.GetString(msg, 0, msg.Length);
        }

        private void fromCAN2UDP()
        {
            byte[] pattern = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
            while (true)
            {
                IPEndPoint remoteIPEndPoint = new IPEndPoint(IPAddress.Any, Cnames.portinCAN);
                try
                {
                    byte[] content = CANServer.Receive(ref remoteIPEndPoint);
                    if (content.Length > 0)
                    {
                        // Das Programm reagiert auf die Erkennung 
                        switch (content[1])
                        {
                            case 0x0F: // ReadConfig_R:
                                {
                                    ConfigStream.getLokName(content[11]);
                                    UpdateProgressMFXBar();
                                }
                                break;
                            case 0x31: // Ping_R:
                                if (receivePINGInfos == true && CANguruArrFilled < 20)
                                {
                                    // CANguru ?
                                    if (content[12] >= DEVTYPE_BASE && content[12] < DEVTYPE_LastCANguru)
                                    {
                                        bool alreadyKnown = false;
                                        for (byte c = 0; c < CANguruArrFilled; c++)
                                        {
                                            if (content[2] == CANguruPINGArr[c, 2] && content[3] == CANguruPINGArr[c, 3])
                                            {
                                                alreadyKnown = true;
                                                break;
                                            }
                                        }
                                        if (alreadyKnown == false)
                                        {
                                            for (byte i = 0; i < Cnames.lngFrame; i++)
                                                CANguruPINGArr[CANguruArrFilled, i] = content[i];
                                            CANguruArrFilled++;
                                        }
                                    }
                                }
                                break;
                            case 0x3B: // Config
                                       // die Zeilen der Pakete werden alle in das Array
                                       // CANguruArr kopiert
                                for (byte i = 0; i < Cnames.lngFrame; i++)
                                    CANguruArr[CANguruArrLine, i] = content[i];
                                CANguruArrLine++;
                                // 0 ist die Gerätebeschreibung (Paket 0)
                                // dieses Paket ist vollständig, wenn die
                                // Länge der Zeile =6 beträgt
                                if (content[0x04] == 6)
                                {
                                    if (CANguruArrIndex == 0)
                                        read1ConfigChannel_DescriptionBlock(ref CANguruArrIndex, ref content);
                                    if (CANguruArrIndex > 0)
                                        read1ConfigChannel_ValueBlock(ref CANguruArrIndex);
                                }
                                break;
                            case 0x40: // ConfigData:
                                if (content[4] == 0x08)
                                {
                                    string loks0 = "loks";
                                    byte[] bloks = new byte[4]; // 5 Zeichen wg. /0 am Ende
                                    Array.Copy(content, 5, bloks, 0, 4);
                                    string loks1 = System.Text.Encoding.Default.GetString(bloks);
                                    if (loks0 == loks1)
                                    {
                                        // Das Programm reagiert auf die Erkennung eines mfx-Dekoders und vergibt eine SID
                                        // Anzahl der Zeichen übermitteln
                                        ConfigStream.generateEmptyLokList();
                                        ConfigStream.readLocomotive(Cnames.path, Cnames.cs2name);
                                        byte[] tmpbyte4 = new byte[4];
                                        tmpbyte4 = BitConverter.GetBytes(IPAddress.HostToNetworkOrder(ConfigStream.getbufferIndex()));
                                        Array.Copy(tmpbyte4, 0, GETCONFIG_RESPONSE, 5, 4);
                                        ChangeMyText(this.TelnetComm, doMsg4TctWindow(CMD.fromGW, GETCONFIG_RESPONSE));
                                        CANClient.Connect(Cnames.IP_CAN, Cnames.portoutCAN);
                                        CANClient.Send(GETCONFIG_RESPONSE, GETCONFIG_RESPONSE.Length);
                                    }
                                }
                                break;
                            case 0x41: // ConfigData_R:
                                if (content[4] == 0x08)
                                {
                                    ConfigStream.sendConfig(content, CANClient);
                                }
                                break;
                            case 0x50: // MfxProc:
                                if (content[4] == 0x01)
                                {
                                    while (myQueue.lngQueue() > 0)
                                    {
                                        pattern = myQueue.eatQueue();
                                        CANClient.Send(pattern, Cnames.lngFrame);
                                    }
                                }
                                break;
                            case 0x51: // MfxProc_R:
                                if (content[5] == 0x01)
                                {
                                    // config stream startet
                                    ConfigStream.startConfigStructmfx(content);
                                }
                                if (content[5] == 0x00)
                                {
                                    // config stream wird beendet
                                    ConfigStream.finishConfigStruct();
                                    //   ConfigStream.incCounter();
                                    //   ConfigStream.incnextLocid();
                                    //   numCounter.Value = ConfigStream.getCounter();
                                    //   numLocID.Value = ConfigStream.getnextLocid();
                                }
                                break;
                            case 0x89:
                                Cnames.IP_CAN = remoteIPEndPoint.Address.ToString();
                                this.txtbHost_CAN.Invoke(new MethodInvoker(() => this.txtbHost_CAN.Text = Cnames.IP_CAN));
                                connectThread = new Thread(this.connectServer);
                                connectThread.Start();
                                is_connected ^= true;
                                Seta1milliTimer();
                                break;
                        }
                    }
                    if (content[0x01] != 0x89)
                        ChangeMyText(this.TelnetComm, doMsg4TctWindow(CMD.toGW, content));
                }
                catch (Exception e)
                {
                    MessageBox.Show("Threat-Exception: " + e.Message);
                }
            }
        }

        private void mfxLoks_Click(object sender, EventArgs e)
        {
            // mfx-discovery
            byte[] MFX_STOP = { 0x00, 0x00, 0x03, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
            byte[] MFX_LOCID = { 0x00, 0x50, 0x03, 0x00, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
            byte[] MFX_MAGIC = { 0x00, 0x36, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
            byte[] MFX_PING = { 0x00, 0x30, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

            ConfigStream.setnextLocid((byte)(numLocID.Value));
            MFX_LOCID[5] = ConfigStream.getnextLocid();

            ChangeMyText(this.TelnetComm, doMsg4TctWindow(CMD.fromGW, MFX_LOCID));
            CANClient.Connect(Cnames.IP_CAN, Cnames.portoutCAN);
            CANClient.Send(MFX_LOCID, Cnames.lngFrame);
            myQueue.fillQueue(MFX_MAGIC);
            myQueue.fillQueue(MFX_PING);
            myQueue.fillQueue(MFX_STOP);
            elapsedsec = 0;
            Seta1secTimer();
            setProgressMFXBar((int)(14));
        }

        private void After1sec(Object source, ElapsedEventArgs e)
        {
            byte[] MFX_COUNTER = { 0x00, 0x00, 0x03, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x09, 0x00, 0x03, 0x00 };
            byte[] MFX_MAGIC = { 0x00, 0x36, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
            byte[] MFX_PING = { 0x00, 0x30, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
            byte[] MFX_GO = { 0x00, 0x00, 0x03, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00 };
            byte[] MFX_UNKNWN = { 0x00, 0x3A, 0x03, 0x00, 0x05, 0x43, 0x54, 0x1C, 0x68, 0x00, 0x00, 0x00, 0x00 };
            elapsedsec++;
            UpdateProgressMFXBar();
            switch (elapsedsec)
            {
                case 1:
                    ConfigStream.setCounter((byte)(numCounter.Value));
                    MFX_COUNTER[11] = ConfigStream.getCounter();
                    ChangeMyText(this.TelnetComm, doMsg4TctWindow(CMD.fromGW, MFX_MAGIC));
                    CANClient.Connect(Cnames.IP_CAN, Cnames.portoutCAN);
                    CANClient.Send(MFX_MAGIC, Cnames.lngFrame);
                    myQueue.fillQueue(MFX_COUNTER);
                    break;
                case 10:
                    ChangeMyText(this.TelnetComm, doMsg4TctWindow(CMD.fromGW, MFX_GO));
                    CANClient.Connect(Cnames.IP_CAN, Cnames.portoutCAN);
                    CANClient.Send(MFX_GO, Cnames.lngFrame);
                    myQueue.fillQueue(MFX_PING);
                    myQueue.fillQueue(MFX_UNKNWN);
                    break;
                case 15:
                    a1secTimer.Enabled = false;
                    break;
            }
        }

        private void Seta1secTimer()
        {
            // Create a timer with a two second interval.
            a1secTimer = new System.Timers.Timer(1000);
            // Hook up the Elapsed event for the timer. 
            a1secTimer.Elapsed += After1sec;
            a1secTimer.AutoReset = true;
            a1secTimer.Enabled = true;
        }

        private void genLokListmfx_Click(object sender, EventArgs e)
        {
            byte[] MFX_LOCLIST = { 0x00, 0x51, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
            // erzeuge neue Liste
            ConfigStream.generateEmptyLokList();
            ConfigStream.generateLokListwithListbox(lokBox);
            // rege CANguru an, neue Liste zu laden
            ChangeMyText(this.TelnetComm, doMsg4TctWindow(CMD.fromGW, MFX_LOCLIST));
            CANClient.Connect(Cnames.IP_CAN, Cnames.portoutCAN);
            CANClient.Send(MFX_LOCLIST, Cnames.lngFrame);
        }

        private void getLok_Click(object sender, EventArgs e)
        {
            string name = txtBoxLokName.Text;
            byte adr = (byte)numLokAdress.Value;
            byte type = (byte)lstBoxDecoderType.SelectedIndex;
            ConfigStream.fillConfigStruct(name, adr, type);
            ConfigStream.finishConfigStruct();
        }

        void findLoks_Click(object sender, EventArgs e)
        {
            ConfigStream.editConfigStruct(lokBox);
        }

        private void delLok_Click(object sender, EventArgs e)
        {
            ConfigStream.delConfigStruct(lokBox);
        }

        private void numCounter_ValueChanged(object sender, EventArgs e)
        {
            ConfigStream.setCounter((byte)(numCounter.Value));
        }

        private void numLocID_ValueChanged(object sender, EventArgs e)
        {
            ConfigStream.setnextLocid((byte)(numLocID.Value));
        }

        private void beenden_Click(object sender, EventArgs e)
        {
            string message = "Wollen Sie wirklich beenden?";
            string caption = "Beenden";
            MessageBoxButtons buttons = MessageBoxButtons.YesNo;
            DialogResult result = MessageBox.Show(this, message, caption, buttons);

            if (result == DialogResult.Yes)
            {
                // Aufräumen
                ini.RemoveAllSections();
                Cnames.IP_CAN = txtbHost_CAN.Text;
                ini.AddSection("IP-address").AddKey("IPCAN").Value = Cnames.IP_CAN;
                ConfigStream.setCounter((byte)(numCounter.Value));
                byte c = ConfigStream.getCounter();
                ini.AddSection("Neuanmeldezaehler").AddKey("Counter").Value = c.ToString();
                ConfigStream.setnextLocid((byte)(numLocID.Value));
                byte n = ConfigStream.getnextLocid();
                ini.AddSection("Lok-Adresse").AddKey("LocID").Value = n.ToString();
                //Save the INI
                ini.Save(string.Concat(Cnames.path, Cnames.ininame));
                voltStop();
                restartTheBridge();
                Environment.Exit(0x1);
            }
        }

        public static String GetMyText(Control ctrl)
        {
            if (ctrl.InvokeRequired)
            {
                var del = new GetMyTextDelegate(GetMyText);
                ctrl.Invoke(del, ctrl);
                return "";
            }
            else
            {
                return ctrl.Text;
            }
        }

        public static void ChangeMyText(TextBox ctrl, string text)
        {
            // toCAN, toClnt, toWDP, fromWDP, fromWDP2CAN, fromCAN2WDP, fromClnt, toGW, fromGW
            String[] source_dest = {
            //   toCAN
                "<CAN<     0x",
            //   toClnt
                "   >Clnt> 0x",
            //   toWDP
                "    >WDP> 0x",
            //   fromWDP
                "    <WDP< 0x",
            //   fromClnt
                "   <Clnt< 0x",
            //   fromWDP2CAN
                "<CAN<WDP< 0x",
            //   fromCAN2WDP
                ">CAN>WDP> 0x",
            //   toGW
                "    >G_W> 0x",
            //   fromGW
                "    <G_W< 0x"
            };
            if (ctrl.InvokeRequired)
            {
                var del = new ChangeMyTextDelegate(ChangeMyText);
                ctrl.Invoke(del, ctrl, text);
            }
            else
            {
                if (text.Length > 1)
                {
                    if (text.StartsWith("&"))
                    {
                        byte[] ch = Encoding.ASCII.GetBytes(text);
                        byte[] arr = new byte[13];
                        if (ch.Length < 28)
                        {
                            for (int i = 0; i < 28; i++)
                                text += "$?";
                            ch = Encoding.ASCII.GetBytes(text);
                        }
                        text = source_dest[ch[1] - '0'];
                        int chPtr;
                        for (int c = 0; c < 13; c++)
                        {
                            chPtr = 2 * c + 2;
                            switch (ch[chPtr])
                            {
                                case 0x24: // $
                                    arr[c] = (byte)ch[chPtr + 1];
                                    break;
                                case 0x25: // %
                                    arr[c] = (byte)(ch[chPtr + 1] - 0x30);
                                    break;
                                case 0x26: // &
                                    arr[c] = (byte)(ch[chPtr + 1] + 0x80);
                                    break;
                            }
                        }
                        StringBuilder builder = new StringBuilder();
                        // Merge all bytes into a string of bytes  
                        builder.Append(arr[0].ToString("X2"));
                        builder.Append("(");
                        builder.Append(arr[1].ToString("X2"));
                        builder.Append(")");
                        for (int i = 2; i < 4; i++)
                        {
                            builder.Append(arr[i].ToString("X2"));
                        }
                        if ((arr[1] & 0x01) == 0x01)
                            builder.Append(" R ");
                        else
                            builder.Append("   ");
                        builder.Append("[");
                        builder.Append(arr[4].ToString("X2"));
                        builder.Append("]");
                        byte[] bytes = new byte[8];
                        for (int i = 5; i < arr.Length; i++)
                        {
                            builder.Append(" ");
                            builder.Append(arr[i].ToString("X2"));
                            bytes[i - 5] = arr[i];
                        }
                        builder.Append(" ");
                        char[] chars = Encoding.UTF8.GetChars(bytes);
                        for (int i = 0; i < chars.Length; i++)
                        {
                            if (chars[i] < ' ' || chars[i] > 'z')
                                builder.Append(".");
                            else
                                builder.Append(chars[i]);
                        }
                        text += builder.ToString();
                    }
                    if (text.StartsWith("!"))
                    {
                        text = text.Substring(1);
                    }
                    if (text.StartsWith("%"))
                    {
                        return;
                    }
                }
                ctrl.AppendText(text);
                ctrl.AppendText(Environment.NewLine);
            }
        }

        private void connectServer()
        {
            try
            {
                this.Client = new TcpClient(Cnames.IP_CAN, Cnames.port);
                stream = this.Client.GetStream();
                reader = new StreamReader(stream);
                var line = "";
                while ((line = reader.ReadLine()) != "END")
                {

                    ChangeMyText(this.TelnetComm, line);
                }
            }

            catch (Exception e)
            {
                MessageBox.Show("ConnectException: " + e.Message);
            }
            finally
            {
            }
        }

        void getConfigData(byte canguru, byte index)
        {
            byte[] GETCONFIG = { 0x00, 0x3A, 0x03, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
            if (canguru >= CANguruArrFilled)
            {
                return;
            }
            // UID eintragen
            for (byte i = 5; i < 9; i++)
                GETCONFIG[i] = CANguruPINGArr[canguru, i];
            // Paketnummer
            GETCONFIG[9] = index;
            ChangeMyText(this.TelnetComm, doMsg4TctWindow(CMD.fromGW, GETCONFIG));
            CANClient.Connect(Cnames.IP_CAN, Cnames.portoutCAN);
            CANClient.Send(GETCONFIG, Cnames.lngFrame);
        }

        private void After1milli(Object source, ElapsedEventArgs e)
        {
            elapsedmillis++;
            switch (elapsedmillis)
            {
                case 1:
                    receivePINGInfos = true;
                    //[maxCANgurus] [maxConfigLines+1] [Cnames.lngFrame]
                    for (byte y = 0; y < Cnames.maxConfigLines; y++)
                        for (byte z = 0; z < Cnames.lngFrame; z++)
                        {
                            CANguruArr[y, z] = 0;
                        }
                    for (byte y = 0; y < Cnames.maxCANgurus; y++)
                        for (byte z = 0; z < Cnames.lngFrame + 1; z++)
                        {
                            CANguruPINGArr[y, z] = 0;
                        }
                    CANguruArrFilled = 0;
                    CANguruArrWorked = 0;
                    CANguruArrLine = 0;
                    CANguruArrIndex = 0;
                    break;
                case 500:
                    a1milliTimer.Enabled = false;
                    receivePINGInfos = false;
                    getConfigData(CANguruArrWorked, CANguruArrIndex);
                    break;
            }
        }

        private void Seta1milliTimer()
        {
            // Create a timer with a 1 miili interval.
            a1milliTimer = new System.Timers.Timer(1);
            // Hook up the Elapsed event for the timer. 
            a1milliTimer.Elapsed += After1milli;
            a1milliTimer.AutoReset = true;
            a1milliTimer.Enabled = true;
            elapsedmillis = 0;
        }

        public void onConnectClick(object sender, EventArgs e)
        {
            scanCANguruBridge();
        }

        static string NetworkGateway()
        {
            string ip = null;
            foreach (NetworkInterface f in NetworkInterface.GetAllNetworkInterfaces())
            {
                if (f.OperationalStatus == OperationalStatus.Up)
                {
                    foreach (GatewayIPAddressInformation d in f.GetIPProperties().GatewayAddresses)
                    {
                        ip = d.Address.ToString();
                    }
                }
            }
            return ip;
        }

        private void setProgressPingBar(int max)
        {
            progressBarPing.Visible = true;
            // Set Minimum to 1 to represent the first file being copied.
            progressBarPing.Minimum = 1;
            // Set Maximum to the total number of files to copy.
            progressBarPing.Maximum = max;
            // Set the initial value of the ProgressBar.
            progressBarPing.Value = 1;
            // Set the Step property to a value of 1 to represent each file being copied.
            progressBarPing.Step = 1;
        }

        private void setProgressMFXBar(int max)
        {
            progressBarMfx.Visible = true;
            // Set Minimum to 1 to represent the first file being copied.
            progressBarMfx.Minimum = 1;
            // Set Maximum to the total number of files to copy.
            progressBarMfx.Maximum = max;
            // Set the initial value of the ProgressBar.
            progressBarMfx.Value = 1;
            // Set the Step property to a value of 1 to represent each file being copied.
            progressBarMfx.Step = 1;
        }

        // your form's methods and properties here, as usual
        private void UpdateProgressPingBar()
        {
            // InvokeRequired required compares the thread ID of the
            // calling thread to the thread ID of the creating thread.
            // If these threads are different, it returns true.
            if (this.progressBarPing.InvokeRequired)
            {
                this.Invoke(UpdateProgressPingBarMethod, new object[] { });
            }
            else
            {
                if (progressBarPing.Value >= progressBarPing.Maximum)
                    progressBarPing.Visible = false;
                else
                    progressBarPing.PerformStep();
            }
        }

        // your form's methods and properties here, as usual
        private void UpdateProgressMFXBar()
        {
            // InvokeRequired required compares the thread ID of the
            // calling thread to the thread ID of the creating thread.
            // If these threads are different, it returns true.
            if (this.progressBarMfx.InvokeRequired)
            {
                this.Invoke(UpdateProgressMFXBarMethod, new object[] { });
            }
            else
            {
                if (progressBarMfx.Value >= progressBarMfx.Maximum)
                    progressBarMfx.Visible = false;
                else
                    progressBarMfx.PerformStep();
            }
        }

        private void scanCANguruBridge()
        {
            byte[] M_SEND_IP = { 0x00, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
            txtbHost_CAN.Text = Cnames.IP_CAN;
            string gate_ip = NetworkGateway();
            string[] array = gate_ip.Split('.');

            Cnames.IP_CAN = array[0] + "." + array[1] + "." + array[2] + "." + 255;
            try
            {
                CANClient.Connect(Cnames.IP_CAN, Cnames.portoutCAN);
            }
            catch (Exception e)
            {
                MessageBox.Show("Scan-Exception: " + e.Message);
            }
            //           ChangeMyText(this.TelnetComm, doMsg4TctWindow(GW2from.fromGW, M_SEND_IP));
            CANClient.Send(M_SEND_IP, Cnames.lngFrame);
        }

        private void voltStop()
        {
            byte[] VOLT_STOP = { 0x00, 0x00, 0x03, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
            CANClient.Connect(Cnames.IP_CAN, Cnames.portoutCAN);
            ChangeMyText(this.TelnetComm, doMsg4TctWindow(CMD.fromGW, VOLT_STOP));
            CANClient.Send(VOLT_STOP, Cnames.lngFrame);
            Voltage = false;
        }
        private void btnVolt_Click(object sender, EventArgs e)
        {
            Button btn = (Button)sender;
            byte[] VOLT_GO = { 0x00, 0x00, 0x03, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00 };

            if (Voltage)
            {
                voltStop();
                btn.Text = "Gleisspannung EIN";
            }
            else
            {
                ChangeMyText(this.TelnetComm, doMsg4TctWindow(CMD.fromGW, VOLT_GO));
                CANClient.Connect(Cnames.IP_CAN, Cnames.portoutCAN);
                CANClient.Send(VOLT_GO, Cnames.lngFrame);
                btn.Text = "Gleisspannung AUS";
                Voltage = true;
            }
        }

        private void showConfigData(byte arrWorked, byte arrIndex)
        {
            byte[] SET_CONFIG = { 0x00, 0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x0B, 0x00, 0x00, 0x00 };
            if (is_connected == false)
                return;
            CANguruArrWorked = arrWorked;
            CANguruArrLine = 0;
            CANguruArrIndex = arrIndex;
            // UID eintragen
            for (byte i = 5; i < 9; i++)
                SET_CONFIG[i] = CANguruPINGArr[lastSelectedItem, i];
            // alte Controls löschen
            byte c = configControls.cntControls;
            for (byte cc = 0; cc < c; cc++)
            {
                // Kanal setzen
                SET_CONFIG[10] = cc;
                SET_CONFIG[10]++;
                // Wert setzen
                SET_CONFIG[11] = 0;
                if (CANguruConfigArr[cc].indicatorNumericUpDown != null)
                {
                    decimal val = CANguruConfigArr[cc].indicatorNumericUpDown.Value;
                    // Get the bytes of the decimal
                    byte[] valBytes = Encoding.ASCII.GetBytes(val.ToString());
                    switch (valBytes.Length)
                    {
                        case 1:
                            int b10 = valBytes[0] - 0x30;
                            SET_CONFIG[12] = (byte)(b10);
                            break;
                        case 2:
                            int b20 = valBytes[1] - 0x30;
                            int b21 = valBytes[0] - 0x30;
                            SET_CONFIG[12] = (byte)(b21 * 10 + b20);
                            break;
                        case 3:
                            int b30 = valBytes[2] - 0x30;
                            int b31 = valBytes[1] - 0x30;
                            int b32 = valBytes[0] - 0x30;
                            int v3 = b32 * 100 + b31 * 10 + b30;
                            SET_CONFIG[11] = (byte)(v3 >> 8);
                            SET_CONFIG[12] = (byte)(v3 & 0x00FF);
                            break;
                        case 4:
                            int b40 = valBytes[3] - 0x30;
                            int b41 = valBytes[2] - 0x30;
                            int b42 = valBytes[1] - 0x30;
                            int b43 = valBytes[0] - 0x30;
                            int v4 = b43 * 1000 + b42 * 100 + b41 * 10 + b40;
                            SET_CONFIG[11] = (byte)(v4 >> 8);
                            SET_CONFIG[12] = (byte)(v4 & 0x00FF);
                            break;
                        default:
                            SET_CONFIG[11] = 0;
                            SET_CONFIG[12] = 0;
                            break;
                    }
                }
                if (CANguruConfigArr[cc].indicatorListBox != null)
                {
                    SET_CONFIG[11] = 0;
                    SET_CONFIG[12] = (byte)CANguruConfigArr[cc].indicatorListBox.SelectedIndex;
                }
                // den (neuen) Wert senden
                ChangeMyText(this.TelnetComm, doMsg4TctWindow(CMD.fromGW, SET_CONFIG));
                CANClient.Connect(Cnames.IP_CAN, Cnames.portoutCAN);
                CANClient.Send(SET_CONFIG, Cnames.lngFrame);
                Task.Delay(50).Wait();
                // Löschen der Controls
                if (Configuration.Controls.Contains(configControls.controlLabel[cc]))
                    Configuration.Controls.Remove(configControls.controlLabel[cc]);
                if (CANguruConfigArr[cc].indicatorNumericUpDown != null)
                {
                    if (Configuration.Controls.Contains(configControls.controlValue[cc]))
                        Configuration.Controls.Remove(configControls.controlValue[cc]);
                }
                if (CANguruConfigArr[cc].indicatorListBox != null)
                {
                    if (Configuration.Controls.Contains(configControls.controlChoice[cc]))
                        Configuration.Controls.Remove(configControls.controlChoice[cc]);
                }
                if (Configuration.Controls.Contains(configControls.controlUnit[cc]))
                    Configuration.Controls.Remove(configControls.controlUnit[cc]);
            }
            // Platz ür neue Controls
            byte maxIndex = CANguruPINGArr[CANguruArrWorked, Cnames.lngFrame];
            CANguruConfigArr = new configStruct[maxIndex];
            configControls.cntControls = maxIndex;
            configControls.controlLabel = new Control[maxIndex];
            configControls.controlUnit = new Control[maxIndex];
            configControls.controlValue = new Control[maxIndex];
            configControls.controlChoice = new Control[maxIndex];
            getConfigData(CANguruArrWorked, CANguruArrIndex);
        }

        private void CANElemente_SelectedIndexChanged(object sender, EventArgs e)
        {
            int curItem = CANElemente.SelectedIndex;
            showConfigData((byte)curItem, 1);
        }

        private void TabControl1_SelectedIndexChanged_1(object sender, EventArgs e)
        {
            if (this.tabControl1.SelectedIndex == 2)
            {
                configControls.cntControls = 0;
                int cnt = CANElemente.Items.Count;
                if (cnt > 0)
                    CANElemente.SetSelected(0, true);
            }
        }

        private void restartTheBridge()
        {
            byte[] RESTART_BRIDGE = { 0x00, 0x62, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
            ChangeMyText(this.TelnetComm, doMsg4TctWindow(CMD.fromGW, RESTART_BRIDGE));
            CANClient.Connect(Cnames.IP_CAN, Cnames.portoutCAN);
            CANClient.Send(RESTART_BRIDGE, Cnames.lngFrame);
            // Alles löschen
            is_connected = false;
            TelnetComm.Clear();
            for (int n = CANElemente.Items.Count - 1; n >= 0; --n)
            {
                CANElemente.Items.RemoveAt(n);
            }
            myQueue.resetQueue();
        }
    }
}
