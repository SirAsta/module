using System;
using System.Diagnostics;
using System.Net.Sockets;
using System.Text;
using System.Windows.Forms;
using System.IO;

namespace UI
{
    public partial class Form1 : Form
    {
        public Form1()
        {
            InitializeComponent();
        }

        private void button1_Click(object sender, EventArgs e)
        {
            string path = Path.Combine(Application.StartupPath, "skidjector.exe");
            if (!File.Exists(path))
                path = Path.Combine(Application.StartupPath, "injector.exe");
            Process.Start(path);
        }

        private void button2_Click(object sender, EventArgs e)
        {
            try
            {
                using (TcpClient client = new TcpClient())
                {
                    client.Connect("127.0.0.1", 9002);
                    string script = richTextBox1.Text;
                    byte[] data = Encoding.UTF8.GetBytes(script);
                    NetworkStream stream = client.GetStream();
                    stream.Write(data, 0, data.Length);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show("Failed to execute: " + ex.Message);
            }
        }
    }
}
