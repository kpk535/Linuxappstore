using System.Windows;
using WindowsGitHubAppStore.ViewModels;

namespace WindowsGitHubAppStore;

public partial class MainWindow : Window
{
    public MainWindow()
    {
        InitializeComponent();
        DataContext = new MainViewModel();
    }
}
