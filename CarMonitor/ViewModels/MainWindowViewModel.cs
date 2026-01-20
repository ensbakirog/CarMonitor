using CarMonitor.Models;
using CarMonitor.Services;
using CommunityToolkit.Mvvm.ComponentModel;
using System.ComponentModel;

namespace CarMonitor.ViewModels;

public partial class MainWindowViewModel : ViewModelBase
{
    private readonly Obd2DataService _obd2Service;

    [ObservableProperty]
    private bool _isConnected;

    public MainWindowViewModel()
    {
        _obd2Service = new Obd2DataService();
        _isConnected = _obd2Service.IsConnected;
        _obd2Service.PropertyChanged += OnObd2ServicePropertyChanged;
    }

    private void OnObd2ServicePropertyChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (e.PropertyName == nameof(Obd2DataService.IsConnected))
        {
            IsConnected = _obd2Service.IsConnected;
        }
    }

    public VehicleData VehicleData => _obd2Service.VehicleData;
    public ChartData ChartData => _obd2Service.ChartData;
    public string ConnectedCarName => "Porsche";

    public override void Dispose()
    {
        _obd2Service.PropertyChanged -= OnObd2ServicePropertyChanged;
        _obd2Service?.Dispose();
        base.Dispose();
    }
}