using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO;
using Microsoft.Tools.WindowsInstallerXml;

namespace pm_installer
{
    public class pm_wix_extension : WixExtension
    {
        private pmi_preprocessor_extension preprocessor_extension;

        public override PreprocessorExtension
           PreprocessorExtension
        {
            get
            {
                if (this.preprocessor_extension == null)
                {
                    this.preprocessor_extension =
                       new pmi_preprocessor_extension();
                }

                return this.preprocessor_extension;
            }
        }
    }

    public class pmi_preprocessor_extension :
        PreprocessorExtension
    {
        private static string[] prefixes = { "pm" };

        public override string[] Prefixes
        {
            get
            {
                return prefixes;
            }
        }

        public override string EvaluateFunction(
           string prefix, string function, string[] args)
        {
            string result = null;

            switch (prefix)
            {
                case "pm":
                    switch (function)
                    {
                        case "CheckFileExists":
                            if (args.Length == 1 && File.Exists(args[0]))
                            {
                                result = "true";
                            }
                            else
                            {
                                result = "false";
                            }
                            break;

                        case "GetConsoleAppPath":
                            if (args.Length == 3)
                            {
                                var slnDir = args[0];
                                var ver = args[1];
                                var arch = args[2];
                                result = Path.GetFullPath(Path.Combine(slnDir, "build", "Release", $"PresentMon-{ver}-{arch}.exe"));
                            }
                            else
                            {
                                result = "";
                            }
                            break;

                        case "GetConsoleAppFileName":
                            if (args.Length == 2)
                            {
                                var ver = args[0];
                                var arch = args[1];
                                result = $"PresentMon-{ver}-{arch}.exe";
                            }
                            else
                            {
                                result = "";
                            }
                            break;
                    }
                    break;
            }
            return result;
        }
    }
}
