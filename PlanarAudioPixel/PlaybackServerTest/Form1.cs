using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

using PlanarAudioPixel;

namespace PlaybackServerTest
{
    public partial class Form1 : Form
    {

        PlaybackServer playbackServer = new PlaybackServer();

        public Form1()
        {
            InitializeComponent();
        }

        private void button1_Click(object sender, EventArgs e)
        {

            try
            {
                playbackServer.Start();
                MessageBox.Show("Server started successfully!");
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.ToString());
            }

        }
    }
}
