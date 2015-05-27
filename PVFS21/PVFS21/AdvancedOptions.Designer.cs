/*********************************************************************
 *
 *    Advanced Options dialog for PVFS21
 *
 ********************************************************************/
namespace PVFS21
{
    partial class AdvancedOptions
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(AdvancedOptions));
            this.label2 = new System.Windows.Forms.Label();
            this.label3 = new System.Windows.Forms.Label();
            this.toolTip1 = new System.Windows.Forms.ToolTip(this.components);
            this.txtNoCompress = new System.Windows.Forms.TextBox();
            this.txtDynamicFiles = new System.Windows.Forms.TextBox();
            this.btnOK = new System.Windows.Forms.Button();
            this.btnDefaults = new System.Windows.Forms.Button();
            this.btnCancel = new System.Windows.Forms.Button();
            this.pnlPVFS2 = new System.Windows.Forms.Panel();
            this.pnlPVFS2.SuspendLayout();
            this.SuspendLayout();
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(3, 19);
            this.label2.Margin = new System.Windows.Forms.Padding(3, 3, 3, 0);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(75, 13);
            this.label2.TabIndex = 3;
            this.label2.Text = "Dynamic Files:";
            this.label2.Click += new System.EventHandler(this.label2_Click);
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(3, 84);
            this.label3.Margin = new System.Windows.Forms.Padding(3, 7, 3, 0);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(93, 13);
            this.label3.TabIndex = 4;
            this.label3.Text = "Do Not Compress:";
            // 
            // toolTip1
            // 
            this.toolTip1.AutoPopDelay = 10000;
            this.toolTip1.InitialDelay = 500;
            this.toolTip1.IsBalloon = true;
            this.toolTip1.ReshowDelay = 100;
            this.toolTip1.ToolTipIcon = System.Windows.Forms.ToolTipIcon.Info;
            this.toolTip1.ToolTipTitle = "Advanced Settings Help";
            // 
            // txtNoCompress
            // 
            this.txtNoCompress.Location = new System.Drawing.Point(6, 100);
            this.txtNoCompress.Name = "txtNoCompress";
            this.txtNoCompress.Size = new System.Drawing.Size(242, 20);
            this.txtNoCompress.TabIndex = 6;
            this.txtNoCompress.Text = "*.inc, snmp.bib";
            this.toolTip1.SetToolTip(this.txtNoCompress, resources.GetString("txtNoCompress.ToolTip"));
            // 
            // txtDynamicFiles
            // 
            this.txtDynamicFiles.Location = new System.Drawing.Point(6, 35);
            this.txtDynamicFiles.Name = "txtDynamicFiles";
            this.txtDynamicFiles.Size = new System.Drawing.Size(242, 20);
            this.txtDynamicFiles.TabIndex = 4;
            this.txtDynamicFiles.Text = "*.htm, *.html, *.cgi, *.xml, *.bin, *.txt";
            this.toolTip1.SetToolTip(this.txtDynamicFiles, "These files will be searched for\r\ndynamic variables and indexed for \r\nyour applic" +
        "ation.  Enter file names or\r\nextensions, separated by commas.");
            this.txtDynamicFiles.TextChanged += new System.EventHandler(this.txtDynamicFiles_TextChanged);
            // 
            // btnOK
            // 
            this.btnOK.DialogResult = System.Windows.Forms.DialogResult.OK;
            this.btnOK.Location = new System.Drawing.Point(12, 174);
            this.btnOK.Margin = new System.Windows.Forms.Padding(3, 10, 3, 3);
            this.btnOK.Name = "btnOK";
            this.btnOK.Size = new System.Drawing.Size(75, 23);
            this.btnOK.TabIndex = 7;
            this.btnOK.Text = "OK";
            this.btnOK.UseVisualStyleBackColor = true;
            this.btnOK.Click += new System.EventHandler(this.btnOK_Click);
            // 
            // btnDefaults
            // 
            this.btnDefaults.Location = new System.Drawing.Point(103, 174);
            this.btnDefaults.Name = "btnDefaults";
            this.btnDefaults.Size = new System.Drawing.Size(75, 23);
            this.btnDefaults.TabIndex = 8;
            this.btnDefaults.Text = "Defaults";
            this.btnDefaults.UseVisualStyleBackColor = true;
            this.btnDefaults.Click += new System.EventHandler(this.btnDefaults_Click);
            // 
            // btnCancel
            // 
            this.btnCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.btnCancel.Location = new System.Drawing.Point(194, 174);
            this.btnCancel.Name = "btnCancel";
            this.btnCancel.Size = new System.Drawing.Size(75, 23);
            this.btnCancel.TabIndex = 9;
            this.btnCancel.Text = "Cancel";
            this.btnCancel.UseVisualStyleBackColor = true;
            // 
            // pnlPVFS2
            // 
            this.pnlPVFS2.Controls.Add(this.label2);
            this.pnlPVFS2.Controls.Add(this.txtDynamicFiles);
            this.pnlPVFS2.Controls.Add(this.label3);
            this.pnlPVFS2.Controls.Add(this.txtNoCompress);
            this.pnlPVFS2.Location = new System.Drawing.Point(12, 12);
            this.pnlPVFS2.Name = "pnlPVFS2";
            this.pnlPVFS2.Size = new System.Drawing.Size(257, 149);
            this.pnlPVFS2.TabIndex = 13;
            // 
            // AdvancedOptions
            // 
            this.AcceptButton = this.btnOK;
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.CancelButton = this.btnCancel;
            this.ClientSize = new System.Drawing.Size(280, 207);
            this.Controls.Add(this.pnlPVFS2);
            this.Controls.Add(this.btnCancel);
            this.Controls.Add(this.btnDefaults);
            this.Controls.Add(this.btnOK);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "AdvancedOptions";
            this.ShowIcon = false;
            this.ShowInTaskbar = false;
            this.SizeGripStyle = System.Windows.Forms.SizeGripStyle.Hide;
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Advanced Settings";
            this.Load += new System.EventHandler(this.AdvancedOptions_Load);
            this.pnlPVFS2.ResumeLayout(false);
            this.pnlPVFS2.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.TextBox txtDynamicFiles;
        private System.Windows.Forms.TextBox txtNoCompress;
        private System.Windows.Forms.ToolTip toolTip1;
        private System.Windows.Forms.Button btnOK;
        private System.Windows.Forms.Button btnDefaults;
        private System.Windows.Forms.Button btnCancel;
        private System.Windows.Forms.Panel pnlPVFS2;
    }
}