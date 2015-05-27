/*********************************************************************
 *
 *    Advanced Options dialog for PVFS21
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
    public partial class AdvancedOptions : Form
    {
        public AdvancedOptions()
        {
            InitializeComponent();
        }

     
        /// <summary>
        /// Restores the default settings
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void btnDefaults_Click(object sender, EventArgs e)
        {
            txtDynamicFiles.Text = "*.htm, *.html, *.cgi, *.xml, *.bin";
            txtNoCompress.Text = "*.inc, snmp.bib";
        }

        /// <summary>
        /// Do manual data marshalling
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void AdvancedOptions_Load(object sender, EventArgs e)
        {
            // Manual data marshalling
            txtDynamicFiles.Text = Settings.Default.DynamicFiles;
            txtNoCompress.Text = Settings.Default.NoCompressFiles;
        }

        /// <summary>
        /// Do manual marshalling data back to settings file
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void btnOK_Click(object sender, EventArgs e)
        {
            Settings.Default.OutputVersion = 2;
            Settings.Default.DynamicFiles = txtDynamicFiles.Text;
            Settings.Default.NoCompressFiles = txtNoCompress.Text;
        }

        private void txtDynamicFiles_TextChanged(object sender, EventArgs e)
        {

        }

        private void label2_Click(object sender, EventArgs e)
        {

        }
    }
}
