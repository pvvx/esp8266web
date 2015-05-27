/*********************************************************************
 *
 *    Main Dialog for PVFS21
 *
 ********************************************************************/
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Threading;
using Allchip;
using System.IO;
using System.Net;

namespace PVFS21
{
    public partial class PVFS21Form : Form
    {
        
        #region Fields
        private int groupBox1Height, groupBox2Height, groupBox3Height, groupBox4Height;
        private String strVersion, strBuildDate, strWebPageDestpathMDD;
        private bool lockDisplay = true;
        private PVFS2WebClient web;
        private bool generationResult;
        private List<string> generateLog;
        #endregion
       
        public PVFS21Form()
        {
            InitializeComponent();
            groupBox1Height = groupBox1.Height;
            groupBox3Height = groupBox3.Height;
            groupBox4Height = groupBox4.Height;
        }

        #region Form Load and Unload Functions
        /// <summary>
        /// Configures the form on load
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void PVFS21Form_Load(object sender, EventArgs e)
        {
            // Load version and build date
            Version ver = System.Reflection.Assembly.GetExecutingAssembly().GetName().Version;
            strVersion = ver.Major + "." + ver.Minor + "." + ver.Build + "." + ver.Revision;
            strBuildDate = new DateTime(2000, 1, 1).AddDays(ver.Build).ToString("MMMM d, yyyy");
            lblAbout.Text = strBuildDate + "\nVersion " + strVersion;
            lblAbout.Margin = new Padding(190 - lblAbout.Width, 3, 0, 0);

            // Marshal in a few config things
            if (Settings.Default.StartWithDirectory)
                radStartDir.Checked = true;
            else
                radStartImg.Checked = true;
           
            txtSourceDir.Text = Settings.Default.SourceDirectory;
            txtSourceImage.Text = Settings.Default.SourceImage;
            txtImageName.Text = Settings.Default.ImageName;
            txtProjectDir.Text = Settings.Default.ProjectDirectory;

            lockDisplay = false;
            CorrectDisplay(sender, e);
        }

        /// <summary>
        /// Saves application settings before exiting
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void PVFS21Form_FormClosing(object sender, FormClosingEventArgs e)
        {
            // Marshal out a few config things
            if (radStartDir.Checked)
                Settings.Default.StartWithDirectory = true;
            else
                Settings.Default.StartWithDirectory = false;
            Settings.Default.OutputFormat = 0;

            Settings.Default.SourceDirectory = txtSourceDir.Text;
            Settings.Default.SourceImage = txtSourceImage.Text;
            Settings.Default.ImageName = txtImageName.Text;
            Settings.Default.ProjectDirectory = txtProjectDir.Text;

            // Save application settings
            global::PVFS21.Settings.Default.Save();
        }
        #endregion

        #region Display Option Manager
        /// <summary>
        /// Makes sure the proper options are visible at all times
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void CorrectDisplay(object sender, EventArgs e)
        {
            if (lockDisplay)
                return;

            lockDisplay = true;
            // Correct the display settings as per MDD requirement

                radStartImg.Enabled = true;
                              
                txtImageName.Visible = true;
                label10.Visible = true;
                lblType.Visible = true;

            // Properly configure the output extension
                lblType.Text = "(*.bin)";

            // Configure upload settings enabled/disbled
            if (chkUpload.Checked || radStartImg.Checked)
            {
                txtUploadDestination.Enabled = true;
                btnUploadSettings.Enabled = true;
            }
            else
            {
                txtUploadDestination.Enabled = false;
                btnUploadSettings.Enabled = false;
            }

            // Show correct input label
            if (radStartImg.Checked)
                lblInput.Text = "Source Image:";
            else
                lblInput.Text = "Source Directory:";

            // Show the correct text on the button
            if (radStartImg.Checked)
                btnGenerate.Text = "Upload";
            else if (chkUpload.Checked)
                btnGenerate.Text = "Generate and Upload";
            else
                btnGenerate.Text = "Generate";

            // Show the correct upload path option
            txtUploadDestination.Text = GetProtocol() + "://" + Settings.Default.UploadUser +
                    "@" + Settings.Default.UploadAddress + "/";
            txtUploadDestination.Text += "          ( ==> to modify ==> )";

            // Show only the appropriate steps
            if (radStartImg.Checked)
            {
                txtSourceImage.Visible = true;
                txtSourceDir.Visible = false;
                chkUpload.Visible = false;
                this.ToggleSteps(0, 0, groupBox4Height);
            }
            else {
                txtSourceImage.Visible = false;
                txtSourceDir.Visible = true;
                chkUpload.Visible = true;
                this.ToggleSteps(groupBox2Height, groupBox3Height, groupBox4Height);
            }

            lockDisplay = false;
        }
        #endregion

        #region Animation Functions
        /// <summary>
        /// Hides or shows the various steps
        /// </summary>
        /// <param name="g2Target">Target height for groupBox2</param>
        /// <param name="g3Target">Target height for groupBox3</param>
        /// <param name="g4Target">Target height for groupBox4</param>
        private void ToggleSteps(int g2Target, int g3Target, int g4Target)
        {
            // Step towards the target height slowly
            while(groupBox3.Height != g3Target ||
                groupBox4.Height != g4Target)
            {
                StepToTarget(groupBox3, g3Target);
                StepToTarget(groupBox4, g4Target);
                this.Refresh();
                Thread.Sleep(20);
            }
        }

        /// <summary>
        /// Steps a GroupBox towards its target size
        /// </summary>
        /// <param name="obj">The GroupBox to step</param>
        /// <param name="target">Target size for the object</param>
        private void StepToTarget(GroupBox obj, int target)
        {
            if (obj.Height == target)
                return;

            int step = (int)((obj.Height - target) * 0.25);
            if (obj.Height > target)
                step += 1;
            else
                step -= 1;

            obj.Height -= step;
            this.Height -= step;
        }
        #endregion

        /// <summary>
        /// Handles the generation when clicked.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void btnGenerate_Click(object sender, EventArgs e)
        {

            // Disable the button
            btnGenerate.Enabled = false;

            // Build an image
            if(radStartDir.Checked)
            {
                //// Make sure the project directory is correct
                //if(txtProjectDir.Text.Contains(txtSourceDir.Text))
                //{
                //    generationResult = false;
                //    generateLog = new List<string>();
                //    generateLog.Add("ERROR: The project directory is located in the source " +
                //        "directory.  The generator cannot run if the image is to be placed " + 
                //        "in the source directory.  Please select the base MPLAB project " + 
                //        "directory before continuing.");
                //    generationResult = false;
                //    ShowResultDialog("The image could not be built.");
                //    return;
                //}

                // Set up an appropriate builder
                PVFSBuilder builder;
                if(Settings.Default.OutputVersion == 2)
                {
                    builder = new PVFS2Builder(txtProjectDir.Text, txtImageName.Text);
                    ((PVFS2Builder)builder).DynamicTypes = Settings.Default.DynamicFiles;
                    ((PVFS2Builder)builder).NonGZipTypes = Settings.Default.NoCompressFiles;
                }
                else
                {
                    builder = new PVFSClassicBuilder(txtProjectDir.Text, txtImageName.Text);
                    ((PVFSClassicBuilder)builder).ReserveBlock = (UInt32)Settings.Default.ReserveBlockClassic;
                }

                // Add the files to the image
                myStatusMsg.Text = "Adding source files to image...";
                builder.AddDirectory(txtSourceDir.Text, "");

                // Generate the image
                myStatusMsg.Text = "Generating output image...";
                myProgress.Value = (chkUpload.Checked) ? 20 : 70;
                generationResult = builder.Generate(PVFSOutputFormat.BIN);

                // Indicate full progress for non-uploads
                myProgress.Value = (chkUpload.Checked) ? 20 : 120;
                Thread.Sleep(10);

                // Retrieve the log
                generateLog = builder.Log;

                // Perform the upload if needed
                if (chkUpload.Checked && generationResult)
                {
                    UploadImage(builder.GeneratedImageFileName);
                }
                else
                {
                    if (generationResult)
                        ShowResultDialog("The WEBFS image was successfully generated.");
                    else
                        ShowResultDialog("Errors were encountered while generating the PVFS image.");
                }
/*
                // Show a warning if index has changed
                if (builder.IndexUpdated)
                {
                    MessageBox.Show("The dynamic variables in your web pages have changed!\n\n" +
                                    "Remember to recompile your MPLAB project before continuing\n" +
                                    "to ensure that the project is in sync.",
                                    "PVFS2 Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                } */
            }
            // This is just an upload
            else
            {
                generationResult = true;
                generateLog = new List<string>();
                UploadImage(txtSourceImage.Text);
            }
        }

        /// <summary>
        /// Upload a file from disk to the board
        /// </summary>
        /// <param name="filename"></param>
        private void UploadImage(String filename)
        {
            if (!File.Exists(filename))
            {
                generateLog.Add("ERROR: Could not open " + filename);
                generationResult = false;
                ShowResultDialog("The image could not be uploaded.");
            }
            else {
                String protocol = GetProtocol();

                FileInfo fileinfo = new FileInfo(filename);
                generateLog.Add("\r\nUploading WEBFS2 image: " + fileinfo.Length + " bytes");

                // Set up web client and the credentials
                web = new PVFS2WebClient();
                if (Settings.Default.UploadUser.Length > 0)
                {
                    web.UseDefaultCredentials = false;
                    web.Credentials = new NetworkCredential(Settings.Default.UploadUser, Settings.Default.UploadPass);
                }

                // Update the status bar display
                myStatusMsg.Text = "Contacting device for upload...";
                myProgress.Style = ProgressBarStyle.Marquee;
                Refresh();

                // Register event handlers and start the upload
                web.UploadProgressChanged += new UploadProgressChangedEventHandler(web_UploadProgressChanged);
                web.UploadFileCompleted += new UploadFileCompletedEventHandler(web_UploadFileCompleted);
                web.UploadFileAsync(new Uri(
                    protocol + "://" +
                    Settings.Default.UploadAddress + "/" +
                    Settings.Default.UploadPath),
                    filename);
            }
        }

        /// <summary>
        /// Handle status updates from the web client
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void web_UploadProgressChanged(object sender, UploadProgressChangedEventArgs e)
        {
            myProgress.Style = ProgressBarStyle.Blocks;
            myStatusMsg.Text = "Uploading image (" + e.BytesSent + " / " + e.TotalBytesToSend + " bytes)";
            if(e.ProgressPercentage < 50 && e.ProgressPercentage >= 0)
                myProgress.Value = 20 + (int)(1.5*e.ProgressPercentage);
            if (e.ProgressPercentage == 50)
            {
                myStatusMsg.Text = "Waiting for upload to complete...";
                myProgress.Style = ProgressBarStyle.Marquee;
            }
        }

        /// <summary>
        /// Handles the completion event from the web client
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void web_UploadFileCompleted(object sender, UploadFileCompletedEventArgs e)
        {
            // First, stop the marquee
            myProgress.Style = ProgressBarStyle.Blocks;
            myProgress.Value = 120;
            myStatusMsg.Text = "Process Complete... See status dialog.";

            // Display the results
            if (e.Error == null)
                ShowResultDialog("The WEBFS image upload was successfully completed.");
            else
            {
                generationResult = false;
                generateLog.Add("\r\nERROR: Could not contact remote device for upload.");
                generateLog.Add("ERROR: " + e.Error.Message);
                ShowResultDialog("The WEBFS image could not be uploaded.");
            }
        }

        /// <summary>
        /// Displays the results of a generation / upload routine
        /// </summary>
        /// <param name="message"></param>
        private void ShowResultDialog(String message)
        {
            LogWindow dlg = new LogWindow();

            if (generationResult)
                dlg.Image = SystemIcons.Asterisk;
            else
                dlg.Image = SystemIcons.Error;

            dlg.Message = message;
            dlg.Log = generateLog;

            // This forces the log window to the top if 
            // the application is behind another.
            this.Focus();

            // Show the log window
            dlg.ShowDialog();

            myProgress.Style = ProgressBarStyle.Blocks;
            myProgress.Value = 0;
            myStatusMsg.Text = "[Generator Idle]";
            btnGenerate.Enabled = true;
        }

        #region Button Handlers
        /// <summary>
        /// Selects the source file or directory
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void btnSourceDir_Click(object sender, EventArgs e)
        {
            if (radStartDir.Checked)
            {
                FolderBrowserDialog dlg = new FolderBrowserDialog();
                dlg.SelectedPath = txtSourceDir.Text;
                dlg.Description = "Select the directory in which your web pages are stored:";
                if (dlg.ShowDialog() == DialogResult.OK)
                    txtSourceDir.Text = dlg.SelectedPath;
                DirectoryInfo dir = new DirectoryInfo(dlg.SelectedPath);
                txtProjectDir.Text = dir.Parent.FullName;
            }
            else
            {
                OpenFileDialog dlg = new OpenFileDialog();
                dlg.Filter = "PVFS Image (*.bin)|*.bin";
                dlg.FileName = txtSourceImage.Text;
                if (dlg.ShowDialog() == DialogResult.OK)
                    txtSourceImage.Text = dlg.FileName;
            }
        }

        /// <summary>
        /// Selects the project directory
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void btnProjectDir_Click(object sender, EventArgs e)
        {
            FolderBrowserDialog dlg = new FolderBrowserDialog();
            dlg.SelectedPath = txtProjectDir.Text;
            dlg.Description = "Select the directory in which your project is located:";
            if (dlg.ShowDialog() == DialogResult.OK)
                txtProjectDir.Text = dlg.SelectedPath;
        }
        
        /// <summary>
        /// Shows the About box when the version label is clicked
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void lblAbout_Click(object sender, EventArgs e)
        {
            AboutBox dlg = new AboutBox();
            dlg.ShowDialog();
        }

        /// <summary>
        /// Shows the upload settings dialog when the button is clicked
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void btnUploadSettings_Click(object sender, EventArgs e)
        {
            UploadSettings dlg = new UploadSettings();
            dlg.ShowDialog();
            CorrectDisplay(sender, e);
        }
        
        #endregion

        /// <summary>
        /// Obtains the protocol to be used for uploading
        /// </summary>
        /// <returns></returns>
        private String GetProtocol()
        {
            // For images being built, base on the output version
            if (radStartDir.Checked)
            {
                if (Settings.Default.OutputVersion == 2)
                    return "http";
                else
                    return "ftp";
            }
            // For images being uploaded from disk, try to read the file header
            // If file does not exist, default to http
            else
            {
                try
                {
                    BinaryReader bin = new BinaryReader(new FileStream(txtSourceImage.Text, FileMode.Open), Encoding.ASCII);
                    if (bin.ReadByte() == (byte)'F' && bin.ReadByte() == (byte)'W' &&
                        bin.ReadByte() == (byte)'E' && bin.ReadByte() == (byte)'B' &&
                        bin.ReadByte() == (byte)0x01)
                    {
                        // Upload an PVFS2 image
                        bin.Close();
                        return "http";
                    }
                    else
                    {
                        // Upload an PVFS Classic image
                        bin.Close();
                        return "ftp";
                    }
                }
                catch
                {
                    // Will trap if the file did not exist or was unreadable
                    return "http";
                }
            }
        }

        private void btnAdvanced_Click(object sender, EventArgs e)
        {
            AdvancedOptions dlg = new AdvancedOptions();
            dlg.ShowDialog();
            CorrectDisplay(sender, e);
        }
    }

    /// <summary>
    /// Overrides the WebClient class to force all FTP connections to passive mode
    /// </summary>
    public class PVFS2WebClient : System.Net.WebClient
    {
        protected override WebRequest GetWebRequest(Uri address)
        {
            WebRequest req = base.GetWebRequest(address);
            if (req is FtpWebRequest)
                ((FtpWebRequest)req).UsePassive = false;
            return req;
        }
    }
}
