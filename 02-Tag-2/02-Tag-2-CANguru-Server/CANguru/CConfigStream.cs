using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net.Sockets;
using System.Text;
using System.Windows.Forms;

namespace CANguruX
{
    class CConfigStream
    {
        Cnames names;
        CUtils utils;
        static byte[] inBuffer;
        static byte[] outBuffer = new byte[0x8000];
        static UInt16 bufferIndex = 0;
        struct lokstruct
        {
            public int first, last;
            public string lokname, sid, mfxuid;
            public byte[] byteArrName, byteArrSid, byteArrMfxuid;
            public int sidlng, mfxuidlng;
            public bool ismfx;
        }
        struct configstruct
        {
            public byte[] session;
            public byte[] locid;
            public byte[] typ;
            public byte[] mfxuid;
            public byte[] lokname;
            public byte nameIndex;
        }
        configstruct[] lokconfig;
        byte Counter;
        byte nextLocid;

        // Public constructor
        public CConfigStream(Cnames n)
        {
            utils = new CUtils();
            names = n;
        }

        public UInt16 getbufferIndex()
        {
            return bufferIndex;
        }

        public void setbufferIndex(UInt16 b)
        {
            bufferIndex = b;
        }

        public byte getCounter()
        {
            return Counter;
        }

        public void setCounter(byte c)
        {
            Counter = c;
        }

        public void incCounter()
        {
            Counter++;
            if (Counter > 99)
                Counter = 5;
        }

        public byte getnextLocid()
        {
            return nextLocid;
        }

        public void setnextLocid(byte n)
        {
            nextLocid = n;
        }

        public void incnextLocid()
        {
            nextLocid++;
            if (nextLocid > 99)
                nextLocid = 5;
        }

        public void readLocomotive(string p, string cname)
        {
            string cs2 = string.Concat(p, cname);
            bool fexists = File.Exists(cs2);
            if (fexists)
            {
                FileStream fs = new FileStream(cs2, FileMode.Open);
                // Read bytes
                bufferIndex = (ushort)fs.Length;
                inBuffer = new byte[bufferIndex];
                fs.Read(inBuffer, 0, bufferIndex);
                fs.Close();
            }
            else
                bufferIndex = 0;
        }

        private static IEnumerable<int> PatternAt(byte[] source, byte[] pattern, int start)
        {
            for (int i = start; i < source.Length; i++)
            {
                if (source.Skip(i).Take(pattern.Length).SequenceEqual(pattern))
                {
                    yield return i;
                }
            }
        }

        private static Array RemoveAt(Array source, int index, int tochange)
        {
            if (tochange == 0)
                return source;
            else
            {
                Array dest = Array.CreateInstance(source.GetType().GetElementType(), source.Length + tochange);
                Array.Copy(source, 0, dest, 0, index);
                if (tochange > 0)
                    Array.Copy(source, index, dest, index + tochange, source.Length - index);
                else
                    Array.Copy(source, index - tochange, dest, index, source.Length - index + tochange);

                return dest;
            }
        }

        private int insertToken(byte patternOffset, byte[] token, int start, byte lng)
        {
            byte[] pattern = { 0x4D, 0x4D, 0x4D, 0x4D };
            for (byte o = 0; o < 4; o++)
                pattern[o] += patternOffset;
            IEnumerable<int> patternfound = PatternAt(inBuffer, pattern, start);
            IEnumerator<int> patternlist = patternfound.GetEnumerator();
            if (patternfound.Count() > 0)
            {
                while (patternlist.MoveNext())
                {
                    // ggf. Platz schaffen
                    int dist = lng - 4;
                    if (dist != 0)
                        inBuffer = (byte[])RemoveAt(inBuffer, patternlist.Current, dist);
                    bufferIndex = (ushort)(inBuffer.Length);
                    // token einfügen
                    Array.Copy(token, 0, inBuffer, patternlist.Current, lng);
                }
            }
            return patternlist.Current;
        }

        private void generateLokConfig(byte index)
        {
            lokconfig[index].session = new byte[2];
            lokconfig[index].locid = new byte[2];
            lokconfig[index].typ = new byte[1];
            lokconfig[index].typ[0] = 0x00;
            Array.Resize(ref lokconfig[index].lokname, 50);
            lokconfig[index].mfxuid = new byte[8];
        }

        public void startConfigStructmfx(byte[] cnt)
        {
            lokconfig = new configstruct[1];
            generateLokConfig(0);
            // neuanmeldezähler
            lokconfig[0].session = utils.num2dec(Counter);
            // LocID
            lokconfig[0].locid = utils.num2hex(cnt[6]);
            lokconfig[0].nameIndex = 0;
            // Mfx-UID
            byte[] tmp = new byte[2];
            for (byte i = 0; i < 4; i++)
            {
                tmp = utils.num2hex(cnt[7 + i]);
                Array.Copy(tmp, 0, lokconfig[0].mfxuid, i * 2, 2);
            }
        }

        static byte[] GetBytes(string str)
        {
            byte[] bytes = Encoding.ASCII.GetBytes(str);
            return bytes;
        }

        public void fillConfigStruct(string name, byte adr, byte type)
        {
            lokconfig = new configstruct[1];
            generateLokConfig(0);
            // neuanmeldezähler
            lokconfig[0].session = utils.num2dec(Counter);
            // LocID
            lokconfig[0].locid = utils.num2hex(adr);
            // typ +1, weil 0 ist mfx vorbehalten
            type++;
            lokconfig[0].typ[0] = type;
            // name
            lokconfig[0].lokname = GetBytes(name);
            lokconfig[0].nameIndex = (byte)name.Length;
            // Mfx-UID
            byte[] tmp = new byte[2];
            tmp = utils.num2hex(adr);
            Array.Copy(tmp, 0, lokconfig[0].mfxuid, 0, 2);
        }

        private byte fillConfigArray(byte startIndx)
        {
            readLocomotive(Cnames.path, Cnames.cfgname);
            byte cntConfig = startIndx;
            IEnumerable<int> patternfound = PatternAt(inBuffer, names.separator(), 0);
            IEnumerator<int> patternlist = patternfound.GetEnumerator();
            if (patternfound.Count() > 0)
            {
                byte entry = 5;
                int start = 0;
                while (patternlist.MoveNext())
                {
                    if (entry > 4)
                    {
                        entry = 0;
                        cntConfig++;
                        Array.Resize(ref lokconfig, cntConfig);
                        generateLokConfig((byte)(cntConfig - 1));
                    }
                    int last = patternlist.Current;
                    // pattern suchen
                    switch (entry)
                    {
                        case 0:
                            Array.Copy(inBuffer, start, lokconfig[cntConfig - 1].session, 0, last - start);
                            break;
                        case 1:
                            Array.Copy(inBuffer, start, lokconfig[cntConfig - 1].locid, 0, last - start);
                            break;
                        case 2:
                            Array.Copy(inBuffer, start, lokconfig[cntConfig - 1].typ, 0, last - start);
                            lokconfig[cntConfig - 1].typ[0] -= 0x30;
                            break;
                        case 3:
                            lokconfig[cntConfig - 1].nameIndex = (byte)(last - start);
                            Array.Copy(inBuffer, start, lokconfig[cntConfig - 1].lokname, 0, lokconfig[cntConfig - 1].nameIndex);
                            break;
                        case 4:
                            Array.Copy(inBuffer, start, lokconfig[cntConfig - 1].mfxuid, 0, last - start);
                            break;
                    }
                    start = last + 1;
                    entry++;
                }
            }
            return cntConfig;
        }

        private void writeConfigStruct(byte cntConfig)
        {
            // array von 0 .. cntConfig nach outbuffer und dann wegschreiben
            string tmp = string.Concat(Cnames.path, Cnames.tmpname);
            using (BinaryWriter writer = new BinaryWriter(File.Open(tmp, FileMode.Create)))
            {
                for (byte cntCfg = 0; cntCfg < cntConfig; cntCfg++)
                {
                    writer.Write(lokconfig[cntCfg].session, 0, lokconfig[cntCfg].session.Length);
                    writer.Write(names.separator(), 0, 1);
                    writer.Write(lokconfig[cntCfg].locid, 0, lokconfig[cntCfg].locid.Length);
                    writer.Write(names.separator(), 0, 1);
                    byte[] t = new byte[1];
                    t[0] = (byte)((byte)lokconfig[cntCfg].typ[0] + (byte)0x30);
                    writer.Write(t, 0, 1);
                    writer.Write(names.separator(), 0, 1);
                    writer.Write(lokconfig[cntCfg].lokname, 0, lokconfig[cntCfg].nameIndex);
                    writer.Write(names.separator(), 0, 1);
                    writer.Write(lokconfig[cntCfg].mfxuid, 0, lokconfig[cntCfg].mfxuid.Length);
                    writer.Write(names.separator(), 0, 1);
                }
                writer.Close();
            }
            string cfg = string.Concat(Cnames.path, Cnames.cfgname);
            string bak = string.Concat(Cnames.path, Cnames.bakname);
            if (File.Exists(tmp) && File.Exists(cfg))
                // Replace the file
                File.Replace(tmp, cfg, bak);
            if (File.Exists(tmp))
                // Replace the file
                File.Move(tmp, cfg); // Rename the oldFileName into newFileName
        }

        public void finishConfigStruct()
        {
            // 1 Lok ist die gerade gefundene
            byte cntConfig = 1;
            if (File.Exists(string.Concat(Cnames.path, Cnames.cfgname)))
            {
                cntConfig = fillConfigArray(cntConfig);
            }
            writeConfigStruct(cntConfig);
            MessageBox.Show("MFX-Lok gefunden", "MFX", MessageBoxButtons.OK);
        }

        byte[] typName(byte typ)
        {
            switch (typ)
            {
                case 0:
                    // mfx
                    byte[] Name0 = { 0x6D, 0x66, 0x78 };
                    return Name0;
                case 1:
                    // mm2_dil8
                    byte[] Name1 = { 0x6D, 0x6D, 0x32, 0x5F, 0x64, 0x69, 0x6C, 0x38 };
                    return Name1;
                case 2:
                    // mm2_dil8
                    byte[] Name2 = { 0x6D, 0x6D, 0x32, 0x5F, 0x64, 0x69, 0x6C, 0x38 };
                    return Name2;
                default:
                    byte[] Name = { 0x6D, 0x6D, 0x32, 0x5F, 0x64, 0x69, 0x6C, 0x38 };
                    return Name;
            };
        }

        public void generateEmptyLokList()
        {
            string f001 = string.Concat(Cnames.path, Cnames.name001);
            bool fexists001 = File.Exists(f001);
            if (!fexists001)
            {
                // Create a string array with the lines of text
                string[] lines = { "[lokomotive]", "version", " .minor=3", "session", " .id=MMMM", " " };
                // Write the string array to a new file .
                using (StreamWriter outputFile = new StreamWriter(f001))
                {
                    foreach (string line in lines)
                        outputFile.WriteLine(line);
                }
            }
            string f002 = string.Concat(Cnames.path, Cnames.name002);
            bool fexists002 = File.Exists(f002);
            if (!fexists002)
            {
                // Create a string array with the lines of text
                string[] lines = { 
                    "lokomotive", " .name=NNNN", " .richtung=1", " .uid=0xOOOO", " .adresse=0xPPPP",
                    " .typ=QQQQ", " .sid=0xRRRR", " .mfxuid=0xSSSS", " .symbol=1", " .av=15", 
                    " .bv=15", " .volume=1", " .vmin=8", " .mfxtyp=10", " .funktionen",
                    " ..nr=0", " .funktionen", " ..nr=1", " .funktionen", " ..nr=2",
                    " .funktionen", " ..nr=3", " .funktionen", " ..nr=4", " .funktionen",
                    " ..nr=5", " .funktionen", " ..nr=6", " .funktionen", " ..nr=7",
                    " .funktionen", " ..nr=8", " .funktionen", " ..nr=9", " .funktionen",
                    " ..nr=10", " .funktionen", " ..nr=11", " .funktionen", " ..nr=12",
                    " .funktionen", " ..nr=13", " .funktionen", " ..nr=14", " .funktionen",
                    " ..nr=15", " " };
                // Write the string array to a new file 2.
                using (StreamWriter outputFile = new StreamWriter(f002))
                {
                    foreach (string line in lines)
                        outputFile.WriteLine(line);
                }
            }
            string fcfg = string.Concat(Cnames.path, Cnames.cfgname);
            bool fexistscfg = File.Exists(fcfg);
            if (!fexistscfg)
            {
                // Create a string array with the lines of text
                //session%adresse%typ%name%mfx-uid%
                string[] lines = { "00%32%1%DB CANguru%4711%", " " };
                // Write the string array to a new file .
                using (StreamWriter outputFile = new StreamWriter(fcfg))
                {
                    foreach (string line in lines)
                        outputFile.WriteLine(line);
                }
                generateLokList();
            }
        }
        public void generateLokList()
        {
            // cfg-Datei einlesen und array füllen
            byte cntConfig = 0;
            if (File.Exists(string.Concat(Cnames.path, Cnames.cfgname)))
            {
                cntConfig = fillConfigArray(cntConfig);
            }
            readLocomotive(Cnames.path, Cnames.name001);
            // MMMM - session
            insertToken(0, lokconfig[0].session, 0, 2);
            Array.Copy(inBuffer, 0, outBuffer, 0, bufferIndex);
            int last = bufferIndex;
            for (byte cntCfg = 0; cntCfg < cntConfig; cntCfg++)
            {
                readLocomotive(Cnames.path, Cnames.name002);
                int offset = 0;
                // NNNN - name
                offset = insertToken(1, lokconfig[cntCfg].lokname, offset, lokconfig[cntCfg].nameIndex);
                if (lokconfig[cntCfg].typ[0] == 0) // mfx
                {
                    // OOOO - .uid (locid  .uid=0x4006 /  .uid=0x40OOOO) insgesamt 4-stellig
                    byte[] uid = new byte[4];
                    uid[0] = 0x34;
                    uid[1] = 0x30;
                    uid[2] = lokconfig[cntCfg].locid[0];
                    uid[3] = lokconfig[cntCfg].locid[1];
                    offset = insertToken(2, uid, offset, 4);
                    // PPPP - .adresse ( .adresse = 0x6 / .adresse = 0xPPPP) n-stellig
                    offset = insertToken(3, lokconfig[cntCfg].locid, offset, 2);
                    // QQQQ - .typName=mfx / .typName=mm2_dil8
                    byte[] name = typName(lokconfig[cntCfg].typ[0]);
                    offset = insertToken(4, name, offset, (byte)name.Length);
                    // RRRR - .sid=0x6 / .sid=0xQQQQ wie adresse
                    offset = insertToken(5, lokconfig[cntCfg].locid, offset, 2);
                    // SSSS - .mfxuid=0x7cfdc941 / .mfxuid=0xRRRR
                    offset = insertToken(6, lokconfig[cntCfg].mfxuid, offset, 8);
                }
                else // andere als mfx
                {
                    // OOOO - .uid (locid  .uid=0x4006 /  .uid=0x40OOOO) insgesamt 4-stellig
                    offset = insertToken(2, lokconfig[cntCfg].locid, offset, 2);
                    // PPPP - .adresse ( .adresse = 0x6 / .adresse = 0xPPPP) n-stellig
                    offset = insertToken(3, lokconfig[cntCfg].locid, offset, 2);
                    // QQQQ - .typName=mfx / .typName=mm2_dil8
                    byte[] name = typName(lokconfig[cntCfg].typ[0]);
                    offset = insertToken(4, name, offset, (byte)name.Length);
                    // RRRR - .sid=0x6 / .sid=0xQQQQ wie adresse
                    offset = insertToken(5, lokconfig[cntCfg].locid, offset, 2);
                    // SSSS - .mfxuid=0x7cfdc941 / .mfxuid=0xRRRR
                    byte[] mfxuid = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
                    offset = insertToken(6, mfxuid, offset, 8);
                }
                Array.Copy(inBuffer, 0, outBuffer, last, bufferIndex);
                last += bufferIndex;
            }
            string tmp = string.Concat(Cnames.path, Cnames.tmpname);
            using (BinaryWriter writer = new BinaryWriter(File.Open(tmp, FileMode.Create)))
            {
                writer.Write(outBuffer, 0, last);
                writer.Flush();
                writer.Close();
            }
            string cs2 = string.Concat(Cnames.path, Cnames.cs2name);
            string bak = string.Concat(Cnames.path, Cnames.bakname);
            if (File.Exists(tmp) && File.Exists(cs2))
                // Replace the file.
                File.Replace(tmp, cs2, bak);
            if (File.Exists(tmp))
                // Replace the file
                File.Move(tmp, cs2); // Rename the oldFileName into newFileName
        }
        public void generateLokListwithListbox(ListBox lb)
        {
            generateLokList();
            editConfigStruct(lb);
            MessageBox.Show("MFX-Lokliste angelegt", "MFX", MessageBoxButtons.OK);
        }

        public void editConfigStruct(ListBox lb)
        {
            lb.Items.Clear();
            // cfg-Datei einlesen und array füllen
            byte cntConfig = 0;
            if (File.Exists(string.Concat(Cnames.path, Cnames.cfgname)))
            {
                cntConfig = fillConfigArray(cntConfig);
            }
            for (byte itemNo = 0; itemNo < cntConfig; itemNo++)
            {
                string name = "Name: " + Encoding.UTF8.GetString(lokconfig[itemNo].lokname, 0, lokconfig[itemNo].nameIndex);
                string locid = "LocID: " + Encoding.UTF8.GetString(lokconfig[itemNo].locid, 0, 2);
                string mfxuid = "mfxUID: " + "0x" + Encoding.UTF8.GetString(lokconfig[itemNo].mfxuid, 0, 8);
                string item = name + " - " + locid + " - " + mfxuid;
                lb.Items.Add(item);
            }
        }

        public void getLokName(byte ch)
        {
            lokconfig[0].lokname[lokconfig[0].nameIndex] = ch;
            lokconfig[0].nameIndex++;
        }

        public void XeditConfigStruct(ListBox lb)
        {
            // zunächst Liste leeren
            lb.Items.Clear();
            // mfx-Loks aus locomotive.cs entnehmen und anzeigen
            // .name=
            byte[] pattern0 = { 0x2E, 0x6E, 0x61, 0x6D, 0x65, 0x3D };
            byte[] pattern1 = { 0x2E, 0x73, 0x69, 0x64, 0x3D };
            byte[] pattern2 = { 0x2E, 0x6D, 0x66, 0x78, 0x75, 0x69, 0x64, 0x3D };
            readLocomotive(Cnames.path, Cnames.cs2name);
            IEnumerable<int> loks = PatternAt(inBuffer, pattern0, 0);
            IEnumerator<int> namelist = loks.GetEnumerator();
            IEnumerable<int> sids = PatternAt(inBuffer, pattern1, 0);
            IEnumerator<int> sidlist = sids.GetEnumerator();
            IEnumerable<int> mfxuids = PatternAt(inBuffer, pattern2, 0);
            IEnumerator<int> mfxuidlist = mfxuids.GetEnumerator();
            int cntLoks = loks.Count();
            lokstruct[] lokarray = new lokstruct[cntLoks];
            const int lokarrayLen = 30;
            int x = 0;
            while (namelist.MoveNext())
            {
                lokarray[x].first = namelist.Current;
                lokarray[x].last = inBuffer.Length;
                if (x > 0)
                    lokarray[x - 1].last = lokarray[x].first;
                x++;
            }
            // statt reset
            namelist = loks.GetEnumerator();
            int j = 0;
            int k = 0;
            bool takeTheNext = true;
            int mfxuid_i = 0;
            while (namelist.MoveNext())
            {
                int name_i = namelist.Current + pattern0.Length;
                // name
                k = 0;
                Array.Resize(ref lokarray[j].byteArrName, lokarrayLen);
                Array.Clear(lokarray[j].byteArrName, 0, lokarrayLen);
                while (inBuffer[name_i] != 0x0a)
                {
                    lokarray[j].byteArrName[k] = inBuffer[name_i];
                    name_i++;
                    k++;
                }
                lokarray[j].lokname = System.Text.Encoding.Default.GetString(lokarray[j].byteArrName, 0, k);
                // sid
                int sid_i = 0;
                if (sidlist.MoveNext())
                    sid_i = sidlist.Current + pattern1.Length;
                k = 0;
                Array.Resize(ref lokarray[j].byteArrSid, lokarrayLen);
                Array.Clear(lokarray[j].byteArrName, 0, lokarrayLen);
                while (inBuffer[sid_i] != 0x0a)
                {
                    lokarray[j].byteArrSid[k] = inBuffer[sid_i];
                    sid_i++;
                    k++;
                }
                lokarray[j].sid = System.Text.Encoding.Default.GetString(lokarray[j].byteArrSid, 0, k);
                lokarray[j].sidlng = k - 2; // ohne 0x !
                                            // mfxuid; könnte nicht immer vorhanden sein!!
                if (takeTheNext)
                {
                    if (mfxuidlist.MoveNext())
                        mfxuid_i = mfxuidlist.Current + pattern2.Length;
                }
                if (mfxuid_i > lokarray[j].first && mfxuid_i < lokarray[j].last)
                {
                    takeTheNext = true;
                    k = 0;
                    Array.Resize(ref lokarray[j].byteArrMfxuid, lokarrayLen);
                    Array.Clear(lokarray[j].byteArrMfxuid, 0, lokarrayLen);
                    while (inBuffer[mfxuid_i] != 0x0a)
                    {
                        lokarray[j].byteArrMfxuid[k] = inBuffer[mfxuid_i];
                        mfxuid_i++;
                        k++;
                    }
                    lokarray[j].mfxuid = System.Text.Encoding.Default.GetString(lokarray[j].byteArrMfxuid, 0, k);
                    lokarray[j].mfxuidlng = k - 2; // ohne 0x !
                    lokarray[j].ismfx = true;
                }
                else
                {
                    takeTheNext = false;
                    lokarray[j].mfxuid = "";
                    lokarray[j].ismfx = false;
                }
                string item = lokarray[j].lokname + " - " + lokarray[j].sid + " - " + lokarray[j].mfxuid;
                lb.Items.Add(item);
                j++;
            }
        }

        public void delConfigStruct(ListBox lb)
        {
            if (lb.Items.Count == 0)
            {
                MessageBox.Show("Bitte erste Lokliste füllen", "Fehler", MessageBoxButtons.OK);
            }
            else
            {
                // Get the currently selected item in the ListBox.
                int index = lb.SelectedIndex;
                //
                if (index == -1)
                    MessageBox.Show("Bitte ein Element auswählen", "Fehler", MessageBoxButtons.OK);
                else
                {
                    byte cntConfig = (byte)lb.Items.Count;
                    for (byte i = 0; i < cntConfig; i++)
                        if (i > index)
                            lokconfig[i - 1] = lokconfig[i];
                    cntConfig--;
                    writeConfigStruct(cntConfig);
                    // Listbox neu laden
                    editConfigStruct(lb);
                }
            }
        }

        public void sendConfig(byte[] cnt, UdpClient CANClient)
        {
            string loks0 = "loks";
            byte[] bloks = new byte[4]; // 5 Zeichen wg. /0 am Ende
            Array.Copy(cnt, 5, bloks, 0, 4);
            string loks1 = System.Text.Encoding.Default.GetString(bloks);
            if (loks0 == loks1)
            {
                byte lineNo = cnt[9];
                int buffLen = BitConverter.ToUInt16(cnt, 10);
                byte[] tmpBuffer = new byte[buffLen];
                Array.Clear(tmpBuffer, 0, buffLen);
                int toCopy = 0;
                int inBufferIndex = lineNo * buffLen;
                if ((bufferIndex - inBufferIndex) < buffLen)
                {
                    toCopy = bufferIndex - inBufferIndex;
                }
                else
                {
                    toCopy = buffLen;
                }
                Array.Copy(inBuffer, inBufferIndex, tmpBuffer, 0, toCopy);
                CANClient.Connect(Cnames.IP_CAN, Cnames.portoutCAN);
                CANClient.Send(tmpBuffer, toCopy);
            }
        }
    }
}
