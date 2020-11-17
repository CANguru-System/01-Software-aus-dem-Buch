using System;

namespace CANguruX
{
    class CQueue
    {
        const int NumberOfItems = 10;
        const int lngFrame = 13;
        static byte[][] theQueue = new byte[NumberOfItems][];
        static int queueLng = 0;

        // Public constructor
        public CQueue()
        {
            queueLng = 0;
            for (int i = 0; i < NumberOfItems; ++i)
            {
                theQueue[i] = new byte[13];
            }
        }

        public void resetQueue()
        {
            queueLng = 0;
        }

        public int lngQueue()
        {
            return queueLng;
        }

        public void fillQueue(byte[] msg)
        {
            theQueue[queueLng] = msg;
            queueLng++;
        }

        public byte[] eatQueue()
        {
            byte[] pattern = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
            queueLng--;
            pattern = theQueue[0];
            // ersten überschreiben und umsortieren
            for (int x = 0; x < queueLng; x++)
            {
                theQueue[x] = theQueue[x + 1];
            }
            return pattern;
        }
    }
}
