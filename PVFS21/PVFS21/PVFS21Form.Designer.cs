/*********************************************************************
 *
 *    Main Dialog for PVFS21
 *
 ********************************************************************/
namespace PVFS21
{
    partial class PVFS21Form
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(PVFS21Form));
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.txtSourceImage = new System.Windows.Forms.TextBox();
            this.btnSourceDir = new System.Windows.Forms.Button();
            this.txtSourceDir = new System.Windows.Forms.TextBox();
            this.lblInput = new System.Windows.Forms.Label();
            this.radStartImg = new System.Windows.Forms.RadioButton();
            this.radStartDir = new System.Windows.Forms.RadioButton();
            this.label5 = new System.Windows.Forms.Label();
            this.groupBox3 = new System.Windows.Forms.GroupBox();
            this.lblType = new System.Windows.Forms.Label();
            this.txtImageName = new System.Windows.Forms.TextBox();
            this.label10 = new System.Windows.Forms.Label();
            this.btnProjectDir = new System.Windows.Forms.Button();
            this.txtProjectDir = new System.Windows.Forms.TextBox();
            this.label9 = new System.Windows.Forms.Label();
            this.groupBox4 = new System.Windows.Forms.GroupBox();
            this.txtUploadDestination = new System.Windows.Forms.TextBox();
            this.chkUpload = new System.Windows.Forms.CheckBox();
            this.label11 = new System.Windows.Forms.Label();
            this.btnUploadSettings = new System.Windows.Forms.Button();
            this.btnGenerate = new System.Windows.Forms.Button();
            this.flowLayoutPanel1 = new System.Windows.Forms.FlowLayoutPanel();
            this.myProgress = new System.Windows.Forms.ProgressBar();
            this.myStatusMsg = new System.Windows.Forms.Label();
            this.lblAbout = new System.Windows.Forms.Label();
            this.myToolTip = new System.Windows.Forms.ToolTip(this.components);
            this.btnAdvanced = new System.Windows.Forms.Button();
            this.groupBox1.SuspendLayout();
            this.groupBox3.SuspendLayout();
            this.groupBox4.SuspendLayout();
            this.flowLayoutPanel1.SuspendLayout();
            this.SuspendLayout();
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.Add(this.txtSourceImage);
            this.groupBox1.Controls.Add(this.btnSourceDir);
            this.groupBox1.Controls.Add(this.txtSourceDir);
            this.groupBox1.Controls.Add(this.lblInput);
            this.groupBox1.Controls.Add(this.radStartImg);
            this.groupBox1.Controls.Add(this.radStartDir);
            this.groupBox1.Controls.Add(this.label5);
            this.groupBox1.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.groupBox1.Location = new System.Drawing.Point(15, 5);
            this.groupBox1.Margin = new System.Windows.Forms.Padding(15, 5, 5, 10);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Padding = new System.Windows.Forms.Padding(5, 5, 15, 10);
            this.groupBox1.Size = new System.Drawing.Size(545, 85);
            this.groupBox1.TabIndex = 5;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Source Settings";
            // 
            // txtSourceImage
            // 
            this.txtSourceImage.AutoCompleteMode = System.Windows.Forms.AutoCompleteMode.SuggestAppend;
            this.txtSourceImage.AutoCompleteSource = System.Windows.Forms.AutoCompleteSource.FileSystem;
            this.txtSourceImage.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.txtSourceImage.Location = new System.Drawing.Point(47, 52);
            this.txtSourceImage.Name = "txtSourceImage";
            this.txtSourceImage.Size = new System.Drawing.Size(389, 20);
            this.txtSourceImage.TabIndex = 6;
            this.txtSourceImage.TextChanged += new System.EventHandler(this.CorrectDisplay);
            // 
            // btnSourceDir
            // 
            this.btnSourceDir.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btnSourceDir.Location = new System.Drawing.Point(442, 50);
            this.btnSourceDir.Name = "btnSourceDir";
            this.btnSourceDir.Size = new System.Drawing.Size(75, 23);
            this.btnSourceDir.TabIndex = 5;
            this.btnSourceDir.Text = "Browse...";
            this.btnSourceDir.UseVisualStyleBackColor = true;
            this.btnSourceDir.Click += new System.EventHandler(this.btnSourceDir_Click);
            // 
            // txtSourceDir
            // 
            this.txtSourceDir.AutoCompleteMode = System.Windows.Forms.AutoCompleteMode.SuggestAppend;
            this.txtSourceDir.AutoCompleteSource = System.Windows.Forms.AutoCompleteSource.FileSystemDirectories;
            this.txtSourceDir.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.txtSourceDir.Location = new System.Drawing.Point(47, 52);
            this.txtSourceDir.Name = "txtSourceDir";
            this.txtSourceDir.Size = new System.Drawing.Size(389, 20);
            this.txtSourceDir.TabIndex = 4;
            this.myToolTip.SetToolTip(this.txtSourceDir, "Selects the source file(s) for the remainder of the process.");
            // 
            // lblInput
            // 
            this.lblInput.AutoSize = true;
            this.lblInput.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lblInput.Location = new System.Drawing.Point(44, 36);
            this.lblInput.Name = "lblInput";
            this.lblInput.Size = new System.Drawing.Size(89, 13);
            this.lblInput.TabIndex = 3;
            this.lblInput.Text = "Source Directory:";
            // 
            // radStartImg
            // 
            this.radStartImg.AutoSize = true;
            this.radStartImg.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.radStartImg.Location = new System.Drawing.Point(274, 16);
            this.radStartImg.Name = "radStartImg";
            this.radStartImg.Size = new System.Drawing.Size(164, 17);
            this.radStartImg.TabIndex = 2;
            this.radStartImg.Text = "Upload an existing BIN image";
            this.myToolTip.SetToolTip(this.radStartImg, "Upload an existing BIN image to a device.");
            this.radStartImg.UseVisualStyleBackColor = true;
            this.radStartImg.CheckedChanged += new System.EventHandler(this.CorrectDisplay);
            // 
            // radStartDir
            // 
            this.radStartDir.AutoSize = true;
            this.radStartDir.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.radStartDir.Location = new System.Drawing.Point(135, 16);
            this.radStartDir.Name = "radStartDir";
            this.radStartDir.Size = new System.Drawing.Size(132, 17);
            this.radStartDir.TabIndex = 1;
            this.radStartDir.TabStop = true;
            this.radStartDir.Text = "Generate a new image";
            this.myToolTip.SetToolTip(this.radStartDir, "Generate a new image from a directory of files.");
            this.radStartDir.UseVisualStyleBackColor = true;
            this.radStartDir.CheckedChanged += new System.EventHandler(this.CorrectDisplay);
            // 
            // label5
            // 
            this.label5.AutoSize = true;
            this.label5.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label5.Location = new System.Drawing.Point(44, 18);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(57, 13);
            this.label5.TabIndex = 0;
            this.label5.Text = "Start With:";
            // 
            // groupBox3
            // 
            this.groupBox3.Controls.Add(this.btnAdvanced);
            this.groupBox3.Controls.Add(this.lblType);
            this.groupBox3.Controls.Add(this.txtImageName);
            this.groupBox3.Controls.Add(this.label10);
            this.groupBox3.Controls.Add(this.btnProjectDir);
            this.groupBox3.Controls.Add(this.txtProjectDir);
            this.groupBox3.Controls.Add(this.label9);
            this.groupBox3.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.groupBox3.Location = new System.Drawing.Point(15, 105);
            this.groupBox3.Margin = new System.Windows.Forms.Padding(15, 5, 15, 5);
            this.groupBox3.Name = "groupBox3";
            this.groupBox3.Padding = new System.Windows.Forms.Padding(5, 5, 15, 10);
            this.groupBox3.Size = new System.Drawing.Size(545, 93);
            this.groupBox3.TabIndex = 7;
            this.groupBox3.TabStop = false;
            this.groupBox3.Text = "Output Files";
            // 
            // lblType
            // 
            this.lblType.AutoSize = true;
            this.lblType.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lblType.Location = new System.Drawing.Point(266, 63);
            this.lblType.Name = "lblType";
            this.lblType.Size = new System.Drawing.Size(34, 13);
            this.lblType.TabIndex = 5;
            this.lblType.Text = "(*.bin)";
            // 
            // txtImageName
            // 
            this.txtImageName.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.txtImageName.Location = new System.Drawing.Point(135, 60);
            this.txtImageName.Name = "txtImageName";
            this.txtImageName.Size = new System.Drawing.Size(125, 20);
            this.txtImageName.TabIndex = 4;
            this.myToolTip.SetToolTip(this.txtImageName, "File name for the image \r\nyou\'d like to create.");
            // 
            // label10
            // 
            this.label10.AutoSize = true;
            this.label10.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label10.Location = new System.Drawing.Point(44, 63);
            this.label10.Name = "label10";
            this.label10.Size = new System.Drawing.Size(70, 13);
            this.label10.TabIndex = 3;
            this.label10.Text = "Image Name:";
            // 
            // btnProjectDir
            // 
            this.btnProjectDir.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btnProjectDir.Location = new System.Drawing.Point(442, 32);
            this.btnProjectDir.Name = "btnProjectDir";
            this.btnProjectDir.Size = new System.Drawing.Size(75, 23);
            this.btnProjectDir.TabIndex = 2;
            this.btnProjectDir.Text = "Browse...";
            this.btnProjectDir.UseVisualStyleBackColor = true;
            this.btnProjectDir.Click += new System.EventHandler(this.btnProjectDir_Click);
            // 
            // txtProjectDir
            // 
            this.txtProjectDir.AutoCompleteMode = System.Windows.Forms.AutoCompleteMode.SuggestAppend;
            this.txtProjectDir.AutoCompleteSource = System.Windows.Forms.AutoCompleteSource.FileSystemDirectories;
            this.txtProjectDir.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.txtProjectDir.Location = new System.Drawing.Point(47, 34);
            this.txtProjectDir.Name = "txtProjectDir";
            this.txtProjectDir.Size = new System.Drawing.Size(389, 20);
            this.txtProjectDir.TabIndex = 1;
            this.myToolTip.SetToolTip(this.txtProjectDir, "Select your bin directory.");
            // 
            // label9
            // 
            this.label9.AutoSize = true;
            this.label9.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label9.Location = new System.Drawing.Point(44, 18);
            this.label9.Name = "label9";
            this.label9.Size = new System.Drawing.Size(88, 13);
            this.label9.TabIndex = 0;
            this.label9.Text = "Project Directory:";
            // 
            // groupBox4
            // 
            this.groupBox4.Controls.Add(this.txtUploadDestination);
            this.groupBox4.Controls.Add(this.chkUpload);
            this.groupBox4.Controls.Add(this.label11);
            this.groupBox4.Controls.Add(this.btnUploadSettings);
            this.groupBox4.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.groupBox4.Location = new System.Drawing.Point(15, 208);
            this.groupBox4.Margin = new System.Windows.Forms.Padding(15, 5, 15, 10);
            this.groupBox4.Name = "groupBox4";
            this.groupBox4.Padding = new System.Windows.Forms.Padding(5, 5, 15, 10);
            this.groupBox4.Size = new System.Drawing.Size(545, 67);
            this.groupBox4.TabIndex = 8;
            this.groupBox4.TabStop = false;
            this.groupBox4.Text = "Upload Settings";
            // 
            // txtUploadDestination
            // 
            this.txtUploadDestination.BackColor = System.Drawing.SystemColors.Window;
            this.txtUploadDestination.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.txtUploadDestination.Location = new System.Drawing.Point(68, 34);
            this.txtUploadDestination.Name = "txtUploadDestination";
            this.txtUploadDestination.ReadOnly = true;
            this.txtUploadDestination.Size = new System.Drawing.Size(368, 20);
            this.txtUploadDestination.TabIndex = 3;
            this.myToolTip.SetToolTip(this.txtUploadDestination, "Your PVFS image will be uploaded \r\nhere.  Use the Settings button to \r\nmodify thi" +
        "s destination.");
            // 
            // chkUpload
            // 
            this.chkUpload.AutoSize = true;
            this.chkUpload.Checked = global::PVFS21.Properties.Settings.Default.UploadImageAfterBuild;
            this.chkUpload.DataBindings.Add(new System.Windows.Forms.Binding("Checked", global::PVFS21.Properties.Settings.Default, "UploadImageAfterBuild", true, System.Windows.Forms.DataSourceUpdateMode.OnPropertyChanged));
            this.chkUpload.Location = new System.Drawing.Point(47, 37);
            this.chkUpload.Name = "chkUpload";
            this.chkUpload.Size = new System.Drawing.Size(15, 14);
            this.chkUpload.TabIndex = 2;
            this.myToolTip.SetToolTip(this.chkUpload, "Select this box to upload \r\nyour image upon generation.");
            this.chkUpload.UseVisualStyleBackColor = true;
            this.chkUpload.CheckedChanged += new System.EventHandler(this.CorrectDisplay);
            // 
            // label11
            // 
            this.label11.AutoSize = true;
            this.label11.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label11.Location = new System.Drawing.Point(44, 18);
            this.label11.Name = "label11";
            this.label11.Size = new System.Drawing.Size(92, 13);
            this.label11.TabIndex = 1;
            this.label11.Text = "Upload Image To:";
            // 
            // btnUploadSettings
            // 
            this.btnUploadSettings.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btnUploadSettings.Location = new System.Drawing.Point(442, 32);
            this.btnUploadSettings.Name = "btnUploadSettings";
            this.btnUploadSettings.Size = new System.Drawing.Size(75, 23);
            this.btnUploadSettings.TabIndex = 0;
            this.btnUploadSettings.Text = "Settings";
            this.btnUploadSettings.UseVisualStyleBackColor = true;
            this.btnUploadSettings.Click += new System.EventHandler(this.btnUploadSettings_Click);
            // 
            // btnGenerate
            // 
            this.btnGenerate.BackColor = System.Drawing.SystemColors.Control;
            this.btnGenerate.Font = new System.Drawing.Font("Microsoft Sans Serif", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btnGenerate.Location = new System.Drawing.Point(210, 288);
            this.btnGenerate.Margin = new System.Windows.Forms.Padding(210, 3, 3, 3);
            this.btnGenerate.Name = "btnGenerate";
            this.btnGenerate.Size = new System.Drawing.Size(160, 25);
            this.btnGenerate.TabIndex = 9;
            this.btnGenerate.Text = "Generate and Upload";
            this.btnGenerate.UseVisualStyleBackColor = false;
            this.btnGenerate.Click += new System.EventHandler(this.btnGenerate_Click);
            // 
            // flowLayoutPanel1
            // 
            this.flowLayoutPanel1.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.flowLayoutPanel1.Controls.Add(this.groupBox1);
            this.flowLayoutPanel1.Controls.Add(this.groupBox3);
            this.flowLayoutPanel1.Controls.Add(this.groupBox4);
            this.flowLayoutPanel1.Controls.Add(this.btnGenerate);
            this.flowLayoutPanel1.Controls.Add(this.myProgress);
            this.flowLayoutPanel1.Controls.Add(this.myStatusMsg);
            this.flowLayoutPanel1.Controls.Add(this.lblAbout);
            this.flowLayoutPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.flowLayoutPanel1.Location = new System.Drawing.Point(0, 0);
            this.flowLayoutPanel1.Name = "flowLayoutPanel1";
            this.flowLayoutPanel1.Size = new System.Drawing.Size(574, 360);
            this.flowLayoutPanel1.TabIndex = 10;
            // 
            // myProgress
            // 
            this.myProgress.Location = new System.Drawing.Point(3, 326);
            this.myProgress.Margin = new System.Windows.Forms.Padding(3, 10, 3, 3);
            this.myProgress.MarqueeAnimationSpeed = 50;
            this.myProgress.Maximum = 120;
            this.myProgress.Name = "myProgress";
            this.myProgress.Size = new System.Drawing.Size(200, 18);
            this.myProgress.TabIndex = 10;
            // 
            // myStatusMsg
            // 
            this.myStatusMsg.AutoSize = true;
            this.myStatusMsg.Location = new System.Drawing.Point(209, 326);
            this.myStatusMsg.Margin = new System.Windows.Forms.Padding(3, 10, 3, 0);
            this.myStatusMsg.Name = "myStatusMsg";
            this.myStatusMsg.Size = new System.Drawing.Size(80, 13);
            this.myStatusMsg.TabIndex = 11;
            this.myStatusMsg.Text = "[Generator Idle]";
            // 
            // lblAbout
            // 
            this.lblAbout.AutoSize = true;
            this.lblAbout.Cursor = System.Windows.Forms.Cursors.Help;
            this.lblAbout.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Underline, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lblAbout.ForeColor = System.Drawing.Color.MediumBlue;
            this.lblAbout.Location = new System.Drawing.Point(392, 319);
            this.lblAbout.Margin = new System.Windows.Forms.Padding(100, 3, 3, 0);
            this.lblAbout.Name = "lblAbout";
            this.lblAbout.Size = new System.Drawing.Size(56, 26);
            this.lblAbout.TabIndex = 12;
            this.lblAbout.Text = "Version\r\nBuild Date";
            this.lblAbout.TextAlign = System.Drawing.ContentAlignment.TopRight;
            this.lblAbout.Click += new System.EventHandler(this.lblAbout_Click);
            // 
            // myToolTip
            // 
            this.myToolTip.AutoPopDelay = 10000;
            this.myToolTip.InitialDelay = 500;
            this.myToolTip.IsBalloon = true;
            this.myToolTip.ReshowDelay = 100;
            this.myToolTip.ToolTipIcon = System.Windows.Forms.ToolTipIcon.Info;
            this.myToolTip.ToolTipTitle = "PVFS Generator Help";
            // 
            // btnAdvanced
            // 
            this.btnAdvanced.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btnAdvanced.Location = new System.Drawing.Point(306, 57);
            this.btnAdvanced.Name = "btnAdvanced";
            this.btnAdvanced.Size = new System.Drawing.Size(125, 23);
            this.btnAdvanced.TabIndex = 6;
            this.btnAdvanced.Text = "Advanced Settings";
            this.btnAdvanced.UseVisualStyleBackColor = true;
            this.btnAdvanced.Click += new System.EventHandler(this.btnAdvanced_Click);
            // 
            // PVFS21Form
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(574, 360);
            this.Controls.Add(this.flowLayoutPanel1);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.Name = "PVFS21Form";
            this.SizeGripStyle = System.Windows.Forms.SizeGripStyle.Hide;
            this.Text = "Web Image Generator";
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.PVFS21Form_FormClosing);
            this.Load += new System.EventHandler(this.PVFS21Form_Load);
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            this.groupBox3.ResumeLayout(false);
            this.groupBox3.PerformLayout();
            this.groupBox4.ResumeLayout(false);
            this.groupBox4.PerformLayout();
            this.flowLayoutPanel1.ResumeLayout(false);
            this.flowLayoutPanel1.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.Label label5;
        private System.Windows.Forms.GroupBox groupBox3;
        private System.Windows.Forms.Button btnSourceDir;
        private System.Windows.Forms.TextBox txtSourceDir;
        private System.Windows.Forms.Label lblInput;
        private System.Windows.Forms.RadioButton radStartImg;
        private System.Windows.Forms.RadioButton radStartDir;
        private System.Windows.Forms.TextBox txtImageName;
        private System.Windows.Forms.Label label10;
        private System.Windows.Forms.Button btnProjectDir;
        private System.Windows.Forms.TextBox txtProjectDir;
        private System.Windows.Forms.Label label9;
        private System.Windows.Forms.GroupBox groupBox4;
        private System.Windows.Forms.TextBox txtUploadDestination;
        private System.Windows.Forms.Label label11;
        private System.Windows.Forms.Button btnUploadSettings;
        private System.Windows.Forms.Button btnGenerate;
        private System.Windows.Forms.FlowLayoutPanel flowLayoutPanel1;
        private System.Windows.Forms.ProgressBar myProgress;
        private System.Windows.Forms.Label myStatusMsg;
        private System.Windows.Forms.ToolTip myToolTip;
        private System.Windows.Forms.Label lblType;
        private System.Windows.Forms.TextBox txtSourceImage;
        private System.Windows.Forms.Label lblAbout;
        private System.Windows.Forms.CheckBox chkUpload;
        private System.Windows.Forms.Button btnAdvanced;
    }
}

