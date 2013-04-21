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
using System.Timers;
using System.Drawing;

namespace Recording_GUI_2
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>

    public partial class MainWindow : Window
    {
        private string file;
        private string filename;
        //For just playing the audio
        private MediaPlayer mplayer = new MediaPlayer();
        //For recording the audio path
        private DispatcherTimer timer = new DispatcherTimer();
        private string mousePositionFileString;
        private Boolean leftMouseDown;

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

        private void timer_Tick(object sender, object e)
        {
            audioMediaElementPosition.Content = audioMediaElement.Position;

            //Only record if the left mouse button is pressed
            if (Mouse.LeftButton != MouseButtonState.Pressed)
            {
                return;
            }
                    
            //Draw the mouse position over time
            Point mousePosition = Mouse.GetPosition(this.recordingCanvas);
            string mousePositionString = mousePosition.ToString();
                    
            if (mousePositionFileString != "")
            {
                mousePositionFileString += ";\n " + mousePositionString;
            }
            else
            {
                mousePositionFileString += mousePositionString;
            }
        }
        
        private void StartTimer()
        {
            timer.Tick += new EventHandler(timer_Tick);
            //50 millisecond interval
            timer.Interval = new TimeSpan(0, 0, 0, 0, 50);
            timer.Start();
        }

        private void StopTimer()
        {
            timer.Stop();
        }

        public void audioMediaElement_MediaOpened(Object sender, RoutedEventArgs e) {
            audioMediaElementLength.Content = audioMediaElement.NaturalDuration.TimeSpan;
        }

        //Called when the audio has finished playing
        //Save the mouse movements to a file here
        public void audioMediaElement_MediaEnded(object sender, RoutedEventArgs e)
        {
            StopTimer();
            string path = "";

            Microsoft.Win32.SaveFileDialog dlg = new Microsoft.Win32.SaveFileDialog();
            dlg.FileName = "Path";
            dlg.DefaultExt = ".text";
            dlg.Filter = "Text documents (.txt)|*.txt";

            // Show save file dialog box
            Nullable<bool> result = dlg.ShowDialog();

            // Process save file dialog box results
            if (result == true)
            {
                // Save document
                path = dlg.FileName;
            }
            else
            {
                MessageBoxResult results = MessageBox.Show("File not saved.");
            }

            using (StreamWriter outfile = new StreamWriter(path, false))
            {
                if (mousePositionFileString != null)
                {
                    outfile.Write(mousePositionFileString.ToString());
                }
                else
                {
                    outfile.Write("");
                }
            }

            MessageBoxResult pathResults = MessageBox.Show("Finished recording audio path.\n File: " + path);
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
            if (file == null)
            {
                return;
            }
            //Play the audio back to the user
            //This player JUST plays the audio, nothing else
            mplayer.Open(new Uri(file, UriKind.Relative));
            mplayer.Play();

            //In time with the audio, show the drawn audio path

        }

        //Record button
        private void Record_Click(Object sender, EventArgs e)
        {
            //Check to make sure a file was selected
            if (file == null)
            {
                MessageBoxResult results = MessageBox.Show("Error: You must select a file first");
                return;
            }

            MessageBoxResult result = MessageBox.Show(this, "Are you sure you want to overwrite the previously recorded path?",
                                                        "Confirmation", MessageBoxButton.OKCancel, MessageBoxImage.Warning);
            if (result == MessageBoxResult.Cancel)
            {
                return;
            }
            
            //Clear the canvas
            recordingCanvas.Children.Clear();
            
            //Re-add the rectangle
            Rectangle rectangle = new Rectangle();
            rectangle.Width = 414;
            rectangle.Height = 241;
            rectangle.Stroke = new SolidColorBrush(Colors.Black);
            rectangle.StrokeThickness = 1;
            rectangle.Fill = new SolidColorBrush(Colors.LightGray);
            Canvas.SetLeft(rectangle, 90);
            Canvas.SetTop(rectangle, 25);
            recordingCanvas.Children.Add(rectangle);

            //Play the audio when the user is drawing a path
            audioMediaElement.Source = new Uri(file, UriKind.Relative);
            mousePositionFileString = "";
  
            StartTimer();
            audioMediaElement.Play();
        }

        //Reset button
        private void Reset_Click(Object sender, EventArgs e)
        {
            audioMediaElement.Pause();

            MessageBoxResult result = MessageBox.Show(this, "If you close this window, all data will be lost.",
             "Confirmation", MessageBoxButton.OKCancel, MessageBoxImage.Warning);
            if (result == MessageBoxResult.Cancel)
            {
                audioMediaElement.Play();
            }
            else
            {
                audioMediaElement.Stop();
            }

        }

        //File Browse Button
        private void FileBrowse_Click(Object sender, EventArgs e)
        {
            // Create OpenFileDialog
            Microsoft.Win32.OpenFileDialog dlg = new Microsoft.Win32.OpenFileDialog();

            // Set filter for file extension and default file extension
            dlg.DefaultExt = ".wav";
            dlg.Filter = "Audio Files (.wav)|*.wav";

            // Display OpenFileDialog by calling ShowDialog method
            Nullable<bool> result = dlg.ShowDialog();

            // Get the selected file and display in TextBox
            if (result == true)
            {
                // Open document
                file = dlg.FileName;
                FileNameTextBox.Text = file;
                filename = System.IO.Path.GetFileName(file);
                FileNameLabel.Content = filename;
            }
        }
        
        private void OnMouseDown(Object sender, EventArgs e) {
            leftMouseDown = true;
        }

        private void OnMouseMove(Object sender, EventArgs e)
        {
            if (timer.IsEnabled)
            {
                if (leftMouseDown == true)
                {
                    drawMousePath();
                }
            }
        }

        private void OnMouseUp(Object sender, EventArgs e)
        {
            leftMouseDown = false;
        }

        //Draws on timer ticks in the canvas
        private void drawMousePath()
        {
            Point mousePosition = Mouse.GetPosition(this.recordingCanvas);
            double X = mousePosition.X;
            double Y = mousePosition.Y;

            Ellipse myEllipse = new Ellipse();
            myEllipse.Stroke = System.Windows.Media.Brushes.Black;
            myEllipse.StrokeThickness = 5;
            myEllipse.Height = 5;
            myEllipse.Width = 5;
            Canvas.SetTop(myEllipse, Y);
            Canvas.SetLeft(myEllipse, X);
            recordingCanvas.Children.Add(myEllipse);
       }
    }
}

