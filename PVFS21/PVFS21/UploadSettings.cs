/*********************************************************************
 *
 *    Upload Settings dialog for PVFS21
 *
 ********************************************************************/
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace PVFS21
{
    public partial class UploadSettings : Form
    {
        public UploadSettings()
        {
            InitializeComponent();
        }

        private void btnDefaults_Click(object sender, EventArgs e)
        {
            txtUploadAddress.Text = "ESP8266";
            txtUploadPath.Text = "fsupload";
            txtUploadUser.Text = "ESP8266";
            txtUploadPass.Text = "0123456789";
        }

        private void btnOK_Click(object sender, EventArgs e)
        {
            Settings.Default.UploadAddress = txtUploadAddress.Text;
            Settings.Default.UploadPath = txtUploadPath.Text;
            Settings.Default.UploadUser = txtUploadUser.Text;
            Settings.Default.UploadPass = txtUploadPass.Text;
        }

        private void UploadSettings_Load(object sender, EventArgs e)
        {
            txtUploadAddress.Text = Settings.Default.UploadAddress;
            txtUploadPath.Text = Settings.Default.UploadPath;
            txtUploadUser.Text = Settings.Default.UploadUser;
            txtUploadPass.Text = Settings.Default.UploadPass;
        }

    }
}
