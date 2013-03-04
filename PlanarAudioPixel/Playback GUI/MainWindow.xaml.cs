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

namespace Playback_GUI
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        //variables
        int row = 3;
        int column = 5;
        int i = 0;
        int j = 0;
        Button[,] closeBtn;
        
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
            
            //rectangles
            closeBtn = new Button[row, column];
            for (i = 0; i < row; i++)
            {
                for (j = 0; j < column; j++)
                {
                    closeBtn[i, j] = new Button();
                    closeBtn[i, j].Content = "X";
                    pixelGrid.Children.Add(closeBtn[i, j]);
                    Grid.SetRow(closeBtn[i,j], i);
                    Grid.SetColumn(closeBtn[i,j], j);
 
                }
            }
            //myWindow.Show();
        }

        private void Button_Click_1(object sender, RoutedEventArgs e)
        {

        }

        private void TextBox_TextChanged_1(object sender, TextChangedEventArgs e)
        {

        }

        private void startTime_TextChanged(object sender, TextChangedEventArgs e)
        {

        }

        private void Slider_ValueChanged_1(object sender, RoutedPropertyChangedEventArgs<double> e)
        {

        }


        
    }
}
