using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.IO;

namespace Recording_GUI_2
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>

    public partial class MainWindow : Window
    {
        private string filename;
        private MediaPlayer mplayer = new MediaPlayer();

        public MainWindow()
        {
            InitializeComponent();

            //If the user presses space, call the Record_Click function
            this.PreviewKeyDown += delegate(object sender, KeyEventArgs e)
            {
                if (e.Key.ToString() == "Space") {
                    this.Record_Click(null, null);   
                }
            };
        }

        private void FileBrowse_Click(Object sender, EventArgs e)
        {
            //FileNameTextBox.Text = "Hello";

            // Create OpenFileDialog
            Microsoft.Win32.OpenFileDialog dlg = new Microsoft.Win32.OpenFileDialog();

            // Set filter for file extension and default file extension
            dlg.DefaultExt = ".mp3";
            dlg.Filter = "Audio Files (.wav)|*.wav";

            // Display OpenFileDialog by calling ShowDialog method
            Nullable<bool> result = dlg.ShowDialog();

            // Get the selected file name and display in a TextBox
            if (result == true)
            {
                // Open document
                filename = dlg.FileName;
                FileNameTextBox.Text = filename;
                string file = System.IO.Path.GetFileName(filename);
                FileNameLabel.Content = file;
            }
        }
        
        //Play button
        private void Play_Click(Object sender, EventArgs e)
        {
            //Check to make sure that a file was selected
            if (filename == null)
            {
                return;
            }
                //Play the audio back to the user
                mplayer.Open(new Uri(filename, UriKind.Relative));
                mplayer.Play();

                //In time with the audio, show the drawn audio path

        }

        private void Record_Click(Object sender, EventArgs e)
        { 
            //Check to make sure a file was selected
            if (filename == null) {
                return;
            }

            //Play the audio when the user is drawing a path
            mplayer.Open(new Uri(filename, UriKind.Relative));
            mplayer.Play();
   
            //Record the user's mouse movement for the audio path
            
         
        }

        private void Reset_Click(Object sender, EventArgs e)
        {
           //Pop up with message to confirm reset without saving
            Reset_Popup.IsOpen = true;
        }

            //Reset popup window
            private void Reset_Yes(Object sender, EventArgs e)
            {
                
            }

            private void Reset_Cancel(Object sender, EventArgs e)
            {

            }
    }
}

