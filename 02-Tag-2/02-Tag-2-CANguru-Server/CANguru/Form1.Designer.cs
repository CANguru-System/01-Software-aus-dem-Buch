namespace CANguruX
{
    partial class Form1
    {
        /// <summary>
        /// Erforderliche Designervariable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Verwendete Ressourcen bereinigen.
        /// </summary>
        /// <param name="disposing">True, wenn verwaltete Ressourcen gelöscht werden sollen; andernfalls False.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Vom Windows Form-Designer generierter Code

        /// <summary>
        /// Erforderliche Methode für die Designerunterstützung.
        /// Der Inhalt der Methode darf nicht mit dem Code-Editor geändert werden.
        /// </summary>
        private void InitializeComponent()
        {
            this.Icon = Properties.Resources.CANguru;
            this.beenden = new System.Windows.Forms.Button();
            this.tabControl1 = new System.Windows.Forms.TabControl();
            this.Telnet = new System.Windows.Forms.TabPage();
            this.groupCommand = new System.Windows.Forms.GroupBox();
            this.progressBarPing = new System.Windows.Forms.ProgressBar();
            this.TelnetComm = new System.Windows.Forms.TextBox();
            this.groupInput = new System.Windows.Forms.GroupBox();
            this.txtbHost_CAN = new System.Windows.Forms.TextBox();
            this.label5 = new System.Windows.Forms.Label();
            this.groupAction = new System.Windows.Forms.GroupBox();
            this.buttonConnect = new System.Windows.Forms.Button();
            this.MFX = new System.Windows.Forms.TabPage();
            this.progressBarMfx = new System.Windows.Forms.ProgressBar();
            this.numLokAdress = new System.Windows.Forms.NumericUpDown();
            this.getLok = new System.Windows.Forms.Button();
            this.lstBoxDecoderType = new System.Windows.Forms.ListBox();
            this.label11 = new System.Windows.Forms.Label();
            this.label10 = new System.Windows.Forms.Label();
            this.label9 = new System.Windows.Forms.Label();
            this.txtBoxLokName = new System.Windows.Forms.TextBox();
            this.genLokListmfx = new System.Windows.Forms.Button();
            this.delLok = new System.Windows.Forms.Button();
            this.numLocID = new System.Windows.Forms.NumericUpDown();
            this.numCounter = new System.Windows.Forms.NumericUpDown();
            this.findLoks = new System.Windows.Forms.Button();
            this.lokBox = new System.Windows.Forms.ListBox();
            this.label3 = new System.Windows.Forms.Label();
            this.label2 = new System.Windows.Forms.Label();
            this.mfxDiscovery = new System.Windows.Forms.Button();
            this.Configuration = new System.Windows.Forms.TabPage();
            this.CANElemente = new System.Windows.Forms.ListBox();
            this.changeElements = new System.Windows.Forms.Button();
            this.btnVolt = new System.Windows.Forms.Button();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.label7 = new System.Windows.Forms.Label();
            this.label8 = new System.Windows.Forms.Label();
            this.numUpDnDecNumber = new System.Windows.Forms.NumericUpDown();
            this.numUpDnDelay = new System.Windows.Forms.NumericUpDown();
            this.btnGetData = new System.Windows.Forms.Button();
            this.btnSetData = new System.Windows.Forms.Button();
            this.tabControl1.SuspendLayout();
            this.Telnet.SuspendLayout();
            this.groupCommand.SuspendLayout();
            this.groupInput.SuspendLayout();
            this.groupAction.SuspendLayout();
            this.MFX.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.numLokAdress)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.numLocID)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.numCounter)).BeginInit();
            this.Configuration.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.numUpDnDecNumber)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.numUpDnDelay)).BeginInit();
            this.SuspendLayout();
            // 
            // beenden
            // 
            this.beenden.Location = new System.Drawing.Point(384, 545);
            this.beenden.Name = "beenden";
            this.beenden.Size = new System.Drawing.Size(90, 25);
            this.beenden.TabIndex = 5;
            this.beenden.Text = "Beenden";
            this.beenden.UseVisualStyleBackColor = true;
            this.beenden.Click += new System.EventHandler(this.beenden_Click);
            // 
            // tabControl1
            // 
            this.tabControl1.Alignment = System.Windows.Forms.TabAlignment.Bottom;
            this.tabControl1.Controls.Add(this.Telnet);
            this.tabControl1.Controls.Add(this.MFX);
            this.tabControl1.Controls.Add(this.Configuration);
            this.tabControl1.Location = new System.Drawing.Point(-1, 2);
            this.tabControl1.Name = "tabControl1";
            this.tabControl1.SelectedIndex = 0;
            this.tabControl1.Size = new System.Drawing.Size(487, 537);
            this.tabControl1.TabIndex = 12;
            this.tabControl1.SelectedIndexChanged += new System.EventHandler(this.TabControl1_SelectedIndexChanged_1);
            // 
            // Telnet
            // 
            this.Telnet.Controls.Add(this.groupCommand);
            this.Telnet.Controls.Add(this.groupInput);
            this.Telnet.Location = new System.Drawing.Point(4, 4);
            this.Telnet.Name = "Telnet";
            this.Telnet.Padding = new System.Windows.Forms.Padding(3);
            this.Telnet.Size = new System.Drawing.Size(479, 511);
            this.Telnet.TabIndex = 1;
            this.Telnet.Text = "Telnet";
            this.Telnet.UseVisualStyleBackColor = true;
            // 
            // groupCommand
            // 
            this.groupCommand.Controls.Add(this.progressBarPing);
            this.groupCommand.Controls.Add(this.TelnetComm);
            this.groupCommand.Location = new System.Drawing.Point(8, 86);
            this.groupCommand.Margin = new System.Windows.Forms.Padding(2);
            this.groupCommand.Name = "groupCommand";
            this.groupCommand.Padding = new System.Windows.Forms.Padding(2);
            this.groupCommand.Size = new System.Drawing.Size(463, 420);
            this.groupCommand.TabIndex = 9;
            this.groupCommand.TabStop = false;
            this.groupCommand.Text = "Command";
            // 
            // progressBarPing
            // 
            this.progressBarPing.Location = new System.Drawing.Point(5, 189);
            this.progressBarPing.Name = "progressBarPing";
            this.progressBarPing.Size = new System.Drawing.Size(449, 23);
            this.progressBarPing.TabIndex = 6;
            // 
            // TelnetComm
            // 
            this.TelnetComm.AcceptsReturn = true;
            this.TelnetComm.Font = new System.Drawing.Font("Courier New", 8.5F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.TelnetComm.Location = new System.Drawing.Point(5, 20);
            this.TelnetComm.Multiline = true;
            this.TelnetComm.Name = "TelnetComm";
            this.TelnetComm.Size = new System.Drawing.Size(450, 395);
            this.TelnetComm.TabIndex = 7;
            // 
            // groupInput
            // 
            this.groupInput.Controls.Add(this.txtbHost_CAN);
            this.groupInput.Controls.Add(this.label5);
            this.groupInput.Controls.Add(this.groupAction);
            this.groupInput.Location = new System.Drawing.Point(8, 8);
            this.groupInput.Margin = new System.Windows.Forms.Padding(2);
            this.groupInput.Name = "groupInput";
            this.groupInput.Padding = new System.Windows.Forms.Padding(2);
            this.groupInput.Size = new System.Drawing.Size(463, 81);
            this.groupInput.TabIndex = 7;
            this.groupInput.TabStop = false;
            this.groupInput.Text = "Connect Information";
            // 
            // txtbHost_CAN
            // 
            this.txtbHost_CAN.ForeColor = System.Drawing.SystemColors.WindowText;
            this.txtbHost_CAN.Location = new System.Drawing.Point(18, 43);
            this.txtbHost_CAN.Name = "txtbHost_CAN";
            this.txtbHost_CAN.Size = new System.Drawing.Size(122, 20);
            this.txtbHost_CAN.TabIndex = 14;
            this.txtbHost_CAN.Text = "192.168.178.54";
            this.txtbHost_CAN.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
            // 
            // label5
            // 
            this.label5.Location = new System.Drawing.Point(15, 17);
            this.label5.Margin = new System.Windows.Forms.Padding(2, 0, 2, 0);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(125, 19);
            this.label5.TabIndex = 2;
            this.label5.Text = "CANguru-Bridge Address";
            // 
            // groupAction
            // 
            this.groupAction.Controls.Add(this.buttonConnect);
            this.groupAction.Location = new System.Drawing.Point(231, 17);
            this.groupAction.Margin = new System.Windows.Forms.Padding(2);
            this.groupAction.Name = "groupAction";
            this.groupAction.Padding = new System.Windows.Forms.Padding(2);
            this.groupAction.Size = new System.Drawing.Size(228, 57);
            this.groupAction.TabIndex = 3;
            this.groupAction.TabStop = false;
            this.groupAction.Text = "Action";
            // 
            // buttonConnect
            // 
            this.buttonConnect.Location = new System.Drawing.Point(62, 23);
            this.buttonConnect.Margin = new System.Windows.Forms.Padding(2);
            this.buttonConnect.Name = "buttonConnect";
            this.buttonConnect.Size = new System.Drawing.Size(128, 25);
            this.buttonConnect.TabIndex = 0;
            this.buttonConnect.Text = "Connect";
            this.buttonConnect.UseVisualStyleBackColor = true;
            this.buttonConnect.Click += new System.EventHandler(this.onConnectClick);
            // 
            // MFX
            // 
            this.MFX.Controls.Add(this.progressBarMfx);
            this.MFX.Controls.Add(this.numLokAdress);
            this.MFX.Controls.Add(this.getLok);
            this.MFX.Controls.Add(this.lstBoxDecoderType);
            this.MFX.Controls.Add(this.label11);
            this.MFX.Controls.Add(this.label10);
            this.MFX.Controls.Add(this.label9);
            this.MFX.Controls.Add(this.txtBoxLokName);
            this.MFX.Controls.Add(this.genLokListmfx);
            this.MFX.Controls.Add(this.delLok);
            this.MFX.Controls.Add(this.numLocID);
            this.MFX.Controls.Add(this.numCounter);
            this.MFX.Controls.Add(this.findLoks);
            this.MFX.Controls.Add(this.lokBox);
            this.MFX.Controls.Add(this.label3);
            this.MFX.Controls.Add(this.label2);
            this.MFX.Controls.Add(this.mfxDiscovery);
            this.MFX.Location = new System.Drawing.Point(4, 4);
            this.MFX.Name = "MFX";
            this.MFX.Padding = new System.Windows.Forms.Padding(3);
            this.MFX.Size = new System.Drawing.Size(479, 511);
            this.MFX.TabIndex = 0;
            this.MFX.Text = "MFX-Ctrl";
            this.MFX.UseVisualStyleBackColor = true;
            // 
            // progressBarMfx
            // 
            this.progressBarMfx.Location = new System.Drawing.Point(15, 244);
            this.progressBarMfx.Name = "progressBarMfx";
            this.progressBarMfx.Size = new System.Drawing.Size(449, 23);
            this.progressBarMfx.TabIndex = 34;
            // 
            // numLokAdress
            // 
            this.numLokAdress.Location = new System.Drawing.Point(133, 205);
            this.numLokAdress.Name = "numLokAdress";
            this.numLokAdress.Size = new System.Drawing.Size(100, 20);
            this.numLokAdress.TabIndex = 33;
            // 
            // getLok
            // 
            this.getLok.Location = new System.Drawing.Point(315, 324);
            this.getLok.Name = "getLok";
            this.getLok.Size = new System.Drawing.Size(90, 25);
            this.getLok.TabIndex = 32;
            this.getLok.Text = "Lok erfassen";
            this.getLok.UseVisualStyleBackColor = true;
            this.getLok.Click += new System.EventHandler(this.getLok_Click);
            // 
            // lstBoxDecoderType
            // 
            this.lstBoxDecoderType.FormattingEnabled = true;
            this.lstBoxDecoderType.Items.AddRange(new object[] {
            "MM2_DIL8",
            "MM1_DIL8"});
            this.lstBoxDecoderType.Location = new System.Drawing.Point(133, 253);
            this.lstBoxDecoderType.Name = "lstBoxDecoderType";
            this.lstBoxDecoderType.Size = new System.Drawing.Size(120, 95);
            this.lstBoxDecoderType.TabIndex = 31;
            // 
            // label11
            // 
            this.label11.AutoSize = true;
            this.label11.Location = new System.Drawing.Point(10, 253);
            this.label11.Name = "label11";
            this.label11.Size = new System.Drawing.Size(62, 13);
            this.label11.TabIndex = 30;
            this.label11.Text = "Dekodertyp";
            // 
            // label10
            // 
            this.label10.AutoSize = true;
            this.label10.Location = new System.Drawing.Point(10, 213);
            this.label10.Name = "label10";
            this.label10.Size = new System.Drawing.Size(45, 13);
            this.label10.TabIndex = 28;
            this.label10.Text = "Adresse";
            // 
            // label9
            // 
            this.label9.AutoSize = true;
            this.label9.Location = new System.Drawing.Point(10, 175);
            this.label9.Name = "label9";
            this.label9.Size = new System.Drawing.Size(35, 13);
            this.label9.TabIndex = 26;
            this.label9.Text = "Name";
            // 
            // txtBoxLokName
            // 
            this.txtBoxLokName.Location = new System.Drawing.Point(133, 169);
            this.txtBoxLokName.Name = "txtBoxLokName";
            this.txtBoxLokName.Size = new System.Drawing.Size(100, 20);
            this.txtBoxLokName.TabIndex = 25;
            // 
            // genLokListmfx
            // 
            this.genLokListmfx.Location = new System.Drawing.Point(315, 97);
            this.genLokListmfx.Name = "genLokListmfx";
            this.genLokListmfx.Size = new System.Drawing.Size(90, 25);
            this.genLokListmfx.TabIndex = 24;
            this.genLokListmfx.Text = "Gen Lok CS2";
            this.genLokListmfx.UseVisualStyleBackColor = true;
            this.genLokListmfx.Click += new System.EventHandler(this.genLokListmfx_Click);
            // 
            // delLok
            // 
            this.delLok.Location = new System.Drawing.Point(133, 94);
            this.delLok.Name = "delLok";
            this.delLok.Size = new System.Drawing.Size(90, 25);
            this.delLok.TabIndex = 23;
            this.delLok.Text = "Del Lok";
            this.delLok.UseVisualStyleBackColor = true;
            this.delLok.Click += new System.EventHandler(this.delLok_Click);
            // 
            // numLocID
            // 
            this.numLocID.Location = new System.Drawing.Point(191, 445);
            this.numLocID.Name = "numLocID";
            this.numLocID.Size = new System.Drawing.Size(40, 20);
            this.numLocID.TabIndex = 22;
            this.numLocID.Click += new System.EventHandler(this.numLocID_ValueChanged);
            // 
            // numCounter
            // 
            this.numCounter.Location = new System.Drawing.Point(133, 445);
            this.numCounter.Name = "numCounter";
            this.numCounter.Size = new System.Drawing.Size(40, 20);
            this.numCounter.TabIndex = 21;
            this.numCounter.Click += new System.EventHandler(this.numCounter_ValueChanged);
            // 
            // findLoks
            // 
            this.findLoks.Location = new System.Drawing.Point(5, 94);
            this.findLoks.Name = "findLoks";
            this.findLoks.Size = new System.Drawing.Size(90, 25);
            this.findLoks.TabIndex = 20;
            this.findLoks.Text = "Show Loks";
            this.findLoks.UseVisualStyleBackColor = true;
            this.findLoks.Click += new System.EventHandler(this.findLoks_Click);
            // 
            // lokBox
            // 
            this.lokBox.FormattingEnabled = true;
            this.lokBox.Location = new System.Drawing.Point(5, 9);
            this.lokBox.Name = "lokBox";
            this.lokBox.Size = new System.Drawing.Size(336, 82);
            this.lokBox.TabIndex = 19;
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(195, 428);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(36, 13);
            this.label3.TabIndex = 14;
            this.label3.Text = "LocID";
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(130, 429);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(44, 13);
            this.label2.TabIndex = 15;
            this.label2.Text = "Counter";
            // 
            // mfxDiscovery
            // 
            this.mfxDiscovery.Location = new System.Drawing.Point(315, 441);
            this.mfxDiscovery.Name = "mfxDiscovery";
            this.mfxDiscovery.Size = new System.Drawing.Size(90, 25);
            this.mfxDiscovery.TabIndex = 12;
            this.mfxDiscovery.Text = "Mfx-Discovery";
            this.mfxDiscovery.UseVisualStyleBackColor = true;
            this.mfxDiscovery.Click += new System.EventHandler(this.mfxLoks_Click);
            // 
            // Configuration
            // 
            this.Configuration.Controls.Add(this.CANElemente);
            this.Configuration.Controls.Add(this.changeElements);
            this.Configuration.Location = new System.Drawing.Point(4, 4);
            this.Configuration.Name = "Configuration";
            this.Configuration.Size = new System.Drawing.Size(479, 511);
            this.Configuration.TabIndex = 3;
            this.Configuration.Text = "Konfiguration";
            this.Configuration.UseVisualStyleBackColor = true;
            // 
            // CANElemente
            // 
            this.CANElemente.FormattingEnabled = true;
            this.CANElemente.Location = new System.Drawing.Point(50, 40);
            this.CANElemente.Name = "CANElemente";
            this.CANElemente.Size = new System.Drawing.Size(182, 95);
            this.CANElemente.TabIndex = 1;
            this.CANElemente.SelectedIndexChanged += new System.EventHandler(this.CANElemente_SelectedIndexChanged);
            // 
            // changeElements
            // 
            this.changeElements.Location = new System.Drawing.Point(0, 0);
            this.changeElements.Name = "changeElements";
            this.changeElements.Size = new System.Drawing.Size(75, 23);
            this.changeElements.TabIndex = 2;
            // 
            // btnVolt
            // 
            this.btnVolt.Location = new System.Drawing.Point(29, 547);
            this.btnVolt.Name = "btnVolt";
            this.btnVolt.Size = new System.Drawing.Size(134, 23);
            this.btnVolt.TabIndex = 20;
            this.btnVolt.Text = "Gleisspannung EIN";
            this.btnVolt.UseVisualStyleBackColor = true;
            this.btnVolt.Click += new System.EventHandler(this.btnVolt_Click);
            // 
            // groupBox1
            // 
            this.groupBox1.Location = new System.Drawing.Point(9, 6);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(449, 360);
            this.groupBox1.TabIndex = 0;
            this.groupBox1.TabStop = false;
            // 
            // label7
            // 
            this.label7.Location = new System.Drawing.Point(0, 0);
            this.label7.Name = "label7";
            this.label7.Size = new System.Drawing.Size(100, 23);
            this.label7.TabIndex = 0;
            // 
            // label8
            // 
            this.label8.Location = new System.Drawing.Point(0, 0);
            this.label8.Name = "label8";
            this.label8.Size = new System.Drawing.Size(100, 23);
            this.label8.TabIndex = 0;
            // 
            // numUpDnDecNumber
            // 
            this.numUpDnDecNumber.Location = new System.Drawing.Point(0, 0);
            this.numUpDnDecNumber.Name = "numUpDnDecNumber";
            this.numUpDnDecNumber.Size = new System.Drawing.Size(120, 20);
            this.numUpDnDecNumber.TabIndex = 0;
            // 
            // numUpDnDelay
            // 
            this.numUpDnDelay.Location = new System.Drawing.Point(0, 0);
            this.numUpDnDelay.Name = "numUpDnDelay";
            this.numUpDnDelay.Size = new System.Drawing.Size(120, 20);
            this.numUpDnDelay.TabIndex = 0;
            // 
            // btnGetData
            // 
            this.btnGetData.Location = new System.Drawing.Point(0, 0);
            this.btnGetData.Name = "btnGetData";
            this.btnGetData.Size = new System.Drawing.Size(75, 23);
            this.btnGetData.TabIndex = 0;
            // 
            // btnSetData
            // 
            this.btnSetData.Location = new System.Drawing.Point(0, 0);
            this.btnSetData.Name = "btnSetData";
            this.btnSetData.Size = new System.Drawing.Size(75, 23);
            this.btnSetData.TabIndex = 0;
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(493, 579);
            this.Controls.Add(this.btnVolt);
            this.Controls.Add(this.beenden);
            this.Controls.Add(this.tabControl1);
            this.Name = "Form1";
            this.Text = "CANguru-Server";
            this.tabControl1.ResumeLayout(false);
            this.Telnet.ResumeLayout(false);
            this.groupCommand.ResumeLayout(false);
            this.groupCommand.PerformLayout();
            this.groupInput.ResumeLayout(false);
            this.groupInput.PerformLayout();
            this.groupAction.ResumeLayout(false);
            this.MFX.ResumeLayout(false);
            this.MFX.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.numLokAdress)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.numLocID)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.numCounter)).EndInit();
            this.Configuration.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.numUpDnDecNumber)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.numUpDnDelay)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion
        private System.Windows.Forms.Button beenden;
        private System.Windows.Forms.TabControl tabControl1;
        private System.Windows.Forms.TabPage MFX;
        private System.Windows.Forms.Button genLokListmfx;
        private System.Windows.Forms.Button delLok;
        private System.Windows.Forms.NumericUpDown numLocID;
        private System.Windows.Forms.NumericUpDown numCounter;
        private System.Windows.Forms.Button findLoks;
        private System.Windows.Forms.ListBox lokBox;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Button mfxDiscovery;
        private System.Windows.Forms.TabPage Telnet;
        private System.Windows.Forms.GroupBox groupCommand;
        private System.Windows.Forms.ProgressBar progressBarPing;
        private System.Windows.Forms.ProgressBar progressBarMfx;
        private System.Windows.Forms.TextBox TelnetComm;
        private System.Windows.Forms.GroupBox groupInput;
        private System.Windows.Forms.Button buttonConnect;
        private System.Windows.Forms.Label label5;
        private System.Windows.Forms.GroupBox groupAction;
        private System.Windows.Forms.Label label9;
        private System.Windows.Forms.TextBox txtBoxLokName;
        private System.Windows.Forms.Button getLok;
        private System.Windows.Forms.ListBox lstBoxDecoderType;
        private System.Windows.Forms.Label label11;
        private System.Windows.Forms.Label label10;
        private System.Windows.Forms.NumericUpDown numLokAdress;
        private System.Windows.Forms.Button btnVolt;
        private System.Windows.Forms.TextBox txtbHost_CAN;
        private System.Windows.Forms.TabPage Configuration;
        private System.Windows.Forms.ListBox CANElemente;
        private System.Windows.Forms.Button changeElements;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.Label label7;
        private System.Windows.Forms.Label label8;
        private System.Windows.Forms.NumericUpDown numUpDnDecNumber;
        private System.Windows.Forms.NumericUpDown numUpDnDelay;
        private System.Windows.Forms.Button btnGetData;
        private System.Windows.Forms.Button btnSetData;
    }
}

