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
using PlanarAudioPixel;

namespace Playback_GUI
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        //variables
        int row = 4;
        int column = 6;
        int i = 0;
        int j = 0;
        Button[,] closeBtn;
        Image[,] pixImg;
        PlaybackServer playbackServer = new PlaybackServer();
        string audioFile;
        string positionFile;
        
        public MainWindow()
        {
            InitializeComponent();
            // Create the Grid
            pixelGrid.ShowGridLines = true;
            for (i = 0; i < row; i++)
            {
                pixelGrid.RowDefinitions.Add(new RowDefinition());
            }
            for (j = 0; j < column; j++)
            {
                pixelGrid.ColumnDefinitions.Add(new ColumnDefinition());
            }
            
            //Buttons
            closeBtn = new Button[row, column];
            pixImg = new Image[row, column];
            //image
            BitmapImage bitmap = new BitmapImage(new Uri("/Resources/audioPixel.bmp", UriKind.Relative));
            Image pixelImage = new Image();
            pixelImage.Source = bitmap;
            pixelImage.Stretch = Stretch.Fill;

            for (i = 0; i < row; i++)
            {
                for (j = 0; j < column; j++)
                {
                    //image
                    pixImg[i, j] = new Image();
                    pixImg[i, j].Source = bitmap;
                    pixImg[i, j].Stretch = Stretch.Fill;

                    //buttons
                    closeBtn[i, j] = new Button();
                    closeBtn[i, j].Height = 50;
                    closeBtn[i, j].Width = 50;
                    closeBtn[i, j].HorizontalAlignment = HorizontalAlignment.Center;
                    closeBtn[i, j].VerticalAlignment = VerticalAlignment.Center;
                    closeBtn[i, j].Content = pixImg[i, j];


                    pixelGrid.Children.Add(closeBtn[i, j]);
                    Grid.SetRow(closeBtn[i,j], i);
                    Grid.SetColumn(closeBtn[i,j], j);
                }
            }
        }

        private void positionBrowseBtn_Click(object sender, RoutedEventArgs e)
        {
            //File Browse Button

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
                positionFile = dlg.FileName;
                positionTextbox.Text = positionFile;
            }

        }

        private void audioBrowseBtn_Click(object sender, RoutedEventArgs e)
        {
            //File Browse Button
       
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
                audioFile = dlg.FileName;
                audioTextbox.Text = audioFile;
            }
       
        }

        private void pauseBtn_Click(object sender, RoutedEventArgs e)
        {
            //call Pause() function
            playbackServer.Pause();
        }

        private void stopBtn_Click(object sender, RoutedEventArgs e)
        {
            //call Stop() function
            playbackServer.Stop();
        }

        private void Button_Click_1(object sender, RoutedEventArgs e)
        {
            //play button pressed
            playbackServer.Play();
        }

        private void TextBox_TextChanged_1(object sender, TextChangedEventArgs e)
        {

        }

        private void TextBox_TextChanged_2(object sender, TextChangedEventArgs e)
        {

        }


        
    }
}
