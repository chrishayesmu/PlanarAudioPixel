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

        private void trackAdded(int audioCode, int positionCode)
        {
            MessageBox.Show("Trackz: " + audioCode.ToString() + " " + positionCode);
        }

        public Form1()
        {
            InitializeComponent();
        }

        private void button1_Click(object sender, EventArgs e)
        {

            try
            {
                playbackServer.ServerStart();
                MessageBox.Show("Server started successfully!");
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.ToString());
            }

        }

        private void button2_Click(object sender, EventArgs e)
        {
            if (openFileDialog1.ShowDialog() == System.Windows.Forms.DialogResult.OK)
            {
                textBox1.Text = openFileDialog1.FileName;
            }
        }

        private void button3_Click(object sender, EventArgs e)
        {
            if (openFileDialog2.ShowDialog() == System.Windows.Forms.DialogResult.OK)
            {
                textBox2.Text = openFileDialog2.FileName;
            }
        }

        private void button4_Click(object sender, EventArgs e)
        {
            try
            {
                playbackServer.AddTrack(textBox1.Text, textBox2.Text, trackAdded);
                MessageBox.Show("Track was added!");
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.ToString());
            }
        }
    }
}
