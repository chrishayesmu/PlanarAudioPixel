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
using System.Windows.Threading;
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
        private DispatcherTimer _timer;

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

        //Slider Controls
        /*
         *      http://msdn.microsoft.com/en-us/library/windows/apps/xaml/hh986967.aspx
         * 
                void videoMediaElement_MediaEnded(object sender, RoutedEventArgs e)
                {
                    StopTimer();
                    timelineSlider.Value = 0.0;
                }

                private void SetupTimer()
                {
                    _timer = new DispatcherTimer();
                    _timer.Interval = TimeSpan.FromSeconds(timelineSlider.StepFrequency);
                    StartTimer();
                }

                private void _timer_Tick(object sender, object e)
                {
                    if (!_sliderpressed)
                    {
                        timelineSlider.Value = mplayer.Position.TotalSeconds;
                    }
                }

                private void StartTimer()
                {
                    _timer.Tick += _timer_Tick;
                    _timer.Start();
                }

                private void StopTimer()
                {
                    _timer.Stop();
                    _timer.Tick -= _timer_Tick;
                }

                private double SliderFrequency(TimeSpan timevalue)
                {
                    double stepfrequency = -1;

                    double absvalue = (int)Math.Round(
                        timevalue.TotalSeconds, MidpointRounding.AwayFromZero);

                    stepfrequency = (int)(Math.Round(absvalue / 100));

                    if (timevalue.TotalMinutes >= 10 && timevalue.TotalMinutes < 30)
                    {
                        stepfrequency = 10;
                    }
                    else if (timevalue.TotalMinutes >= 30 && timevalue.TotalMinutes < 60)
                    {
                        stepfrequency = 30;
                    }
                    else if (timevalue.TotalHours >= 1)
                    {
                        stepfrequency = 60;
                    }

                    if (stepfrequency == 0) stepfrequency += 1;

                    if (stepfrequency == 1)
                    {
                        stepfrequency = absvalue / 100;
                    }

                    return stepfrequency;
                }
                */
        //Called when audio is playing
        //Record mouse movements here
        void audioMediaElement_MediaOpened(object sender, RoutedEventArgs e)
        {
            string mousePosition = Mouse.GetPosition(Application.Current.MainWindow).ToString();
            MessageBoxResult result = MessageBox.Show(mousePosition);
            MessageBoxResult results = MessageBox.Show("Audio is playing");
        }

        //Called when the audio has finished playing
        //Save the mouse movements to a file here
        public void audioMediaElement_MediaEnded(object sender, RoutedEventArgs e)
        {
            //StopTimer();
            timelineSlider.Value = 0.0;
            MessageBoxResult results = MessageBox.Show("Audio finished playing");
        }

        //Called when there is an error opening the file
        public void audioMediaElement_MediaFailed(object sender, ExceptionRoutedEventArgs e)
        {
            MessageBoxResult results = MessageBox.Show("Error: Audio could not be opened");
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
            //This player JUST plays the audio, nothing else
            mplayer.Open(new Uri(filename, UriKind.Relative));
            mplayer.Play();

            /*
            audioMediaElement.Source = new Uri(filename,  UriKind.Relative);
            audioMediaElement.Play();
            */

            //In time with the audio, show the drawn audio path

            //Start the slider
            /*double absvalue = (int)Math.Round(
            mplayer.NaturalDuration.TimeSpan.TotalSeconds,
            MidpointRounding.AwayFromZero);

                timelineSlider.Maximum = absvalue;
                timelineSlider.StepFrequency = SliderFrequency(videoMediaElement.NaturalDuration.TimeSpan);
                SetupTimer();*/
        }

        //Record button
        private void Record_Click(Object sender, EventArgs e)
        {
            //Check to make sure a file was selected
            if (filename == null)
            {
                MessageBoxResult results = MessageBox.Show("Error: You must select a file first");
                return;
            }

            //Play the audio when the user is drawing a path
            audioMediaElement.Source = new Uri(filename, UriKind.Relative);
            audioMediaElement.Play();

            //Record the user's mouse movement for the audio path
            //This is done in the audioMediaElement_MediaOpened function

            //Start the slider
            /*double absvalue = (int)Math.Round(
            mplayer.NaturalDuration.TimeSpan.TotalSeconds, MidpointRounding.AwayFromZero);

                        timelineSlider.Maximum = absvalue;
                        timelineSlider.StepFrequency = SliderFrequency(videoMediaElement.NaturalDuration.TimeSpan);
                        SetupTimer();*/
        }

        //Reset button
        private void Reset_Click(Object sender, EventArgs e)
        {
            audioMediaElement.Stop();
            MessageBoxResult result = MessageBox.Show(this, "If you close this window, all data will be lost.",
             "Confirmation", MessageBoxButton.OKCancel, MessageBoxImage.Warning);
            if (result == MessageBoxResult.OK)
            {
                // Yes code here
            }
            else
            {
                // No code here
            }

        }

        //File Browse Button
        private void FileBrowse_Click(Object sender, EventArgs e)
        {

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
    }
}

