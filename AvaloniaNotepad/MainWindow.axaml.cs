using Avalonia.Controls;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using System.IO;
using System.Threading.Tasks;

namespace AvaloniaNotepad
{
    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            InitializeComponent();
            DataContext = new MainWindowViewModel(this);
        }
    }

    public partial class MainWindowViewModel : ObservableObject
    {
        private readonly Window _owner;
        [ObservableProperty]
        private string _text = "";

        public MainWindowViewModel(Window owner)
        {
            _owner = owner;
        }

        [RelayCommand]
        private async Task OpenFile()
        {
            var dialog = new OpenFileDialog();
            var result = await dialog.ShowAsync(_owner);
            if (result != null && result.Length > 0)
            {
                Text = await File.ReadAllTextAsync(result[0]);
            }
        }

        [RelayCommand]
        private async Task SaveFile()
        {
            var dialog = new SaveFileDialog();
            var result = await dialog.ShowAsync(_owner);
            if (result != null)
            {
                await File.WriteAllTextAsync(result, Text);
            }
        }

        [RelayCommand]
        private void Exit()
        {
            _owner.Close();
        }

        [RelayCommand]
        private async Task About()
        {
            var dialog = new Window
            {
                Title = "About AvaloniaNotepad",
                Content = new TextBlock { Text = "AvaloniaNotepad\n\n© 2024" },
                Width = 200,
                Height = 100
            };
            await dialog.ShowDialog(_owner);
        }
    }
}
