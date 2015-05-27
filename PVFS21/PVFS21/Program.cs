/*********************************************************************
 *
 *    Application Launch Point and CLI for PVFS21
 *
 ********************************************************************/
using System;
using System.Collections.Generic;
using System.Windows.Forms;
using System.Drawing;
using Allchip;

namespace PVFS21
{
	
    static class Program
    {
        
        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [STAThread]
        static void Main(String[] args)
        {
        	// If no arguments, show the GUI
            if (args.Length == 0)
            {
                Application.EnableVisualStyles();
                Application.SetCompatibleTextRenderingDefault(false);
                Application.Run(new PVFS21Form());
            }
            // Operate in command line mode
            else
            {

                // Make sure we got at least 3 parameters
                if (args.Length < 3)
                {
                    MessageBox.Show(
                        "Usage: WEBFS [options] <SourceDir> <ProjectDir> <OutputFile>\n" +
                        "    /html \"...\"\t(/h)\t: Dynamic file types (\"*.htm, *.html, *.xml, *.cgi\")\n" +
                        "    /xgzip \"...\"\t(/z)\t: Non-compressible types (\"snmp.bib, *.inc\")\n\n" +
                        "SourceDir, ProjectDir, and OutputFile are required and should be enclosed in quotes.\n" +
                        "OutputFile is placed relative to ProjectDir and *CANNOT* be a full path name.",
                        "WEBFS Console Error", MessageBoxButtons.OK, MessageBoxIcon.Stop);
                    return;
                }

                // Locate the parameters
                String sourceDir = args[args.Length - 3];
                String projectDir = args[args.Length - 2];
                String outputFile = args[args.Length - 1];

                // Set up some defaults
                PVFSOutputFormat fmt = PVFSOutputFormat.BIN;
                byte version = 2;
                int reserveBlock = 64;
                String htmlTypes = "*.htm, *.html, *.xml, *.cgi, *.bin, *.txt";
                String noGZipTypes = "*.inc, snmp.bib";

                // Process each command line argument
                for(int i =0; i < args.Length - 3; i++)
                {
                    String arg = args[i].ToLower();

			        // Check for output format parameters
			        fmt = PVFSOutputFormat.BIN;
			        version = 2;

                    // Check for string parameters
			        if(arg == "/html" || arg == "-h")
				        htmlTypes = args[++i];
			        else if(arg == "/xgzip" || arg == "-z")
				        noGZipTypes = args[++i];

                    else
                    {
                        MessageBox.Show("The command-line option \""+arg+"\" was not recognized.",
					        "WEBFS Console Error",MessageBoxButtons.OK,MessageBoxIcon.Error);
				        return;
                    }
                }

                // Set up an appropriate builder
                PVFSBuilder builder;
                // This is a dummy string , will be initialized when MDD is supported from command line
                String dummy = "Dummy";      
                if (version == 2)
                {
                    builder = new PVFS2Builder(projectDir, outputFile);
                    ((PVFS2Builder)builder).DynamicTypes = htmlTypes;
                    ((PVFS2Builder)builder).NonGZipTypes = noGZipTypes;
                }
                else
                {
                    builder = new PVFSClassicBuilder(projectDir, outputFile);
                    ((PVFSClassicBuilder)builder).ReserveBlock = (UInt32)reserveBlock;
                }

                // Add the files to the image and generate the image
                builder.AddDirectory(sourceDir, "");

                // Generate the image and trap errors
                if (!builder.Generate(fmt))
                {
                    LogWindow dlg = new LogWindow();
                    dlg.Image = SystemIcons.Error;
                    dlg.Message = "An error was encountered during generation.";
                    dlg.Log = builder.Log;
                    dlg.ShowDialog();
                    return;
                }
            }
        }
    }
}
