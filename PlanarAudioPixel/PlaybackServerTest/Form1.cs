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

        private void updateConnectedClients(PlanarAudioPixel.Client client)
        {
            listBox1.Items.Add("" + client.ClientID.LocalIP.Byte1 + "." + client.ClientID.LocalIP.Byte2 + "." + client.ClientID.LocalIP.Byte3 + "." + client.ClientID.LocalIP.Byte4);
        }
        delegate void updateClientGUI(PlanarAudioPixel.Client client);

        private void trackAdded(int audioCode, int positionCode)
        {
            MessageBox.Show("Tracks: " + audioCode.ToString() + " " + positionCode);
        }
        private void clientConnected(PlanarAudioPixel.Client client)
        {
            updateClientGUI updateDelegate = updateConnectedClients;
            this.Invoke(updateDelegate, new object[] { client });
        }
        private void clientDisconnected(PlanarAudioPixel.Client client)
        {
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

                playbackServer.OnClientConnected(clientConnected);
                playbackServer.OnClientDisconnected(clientDisconnected);
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

        private void button5_Click(object sender, EventArgs e)
        {
            try
            {
                playbackServer.Play();
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.ToString());
            }
        }
    }
}
