using CarMonitor.Models;
using CarMonitor.Services;

namespace CarMonitor.ViewModels;

public partial class MainWindowViewModel : ViewModelBase
{
    private readonly Obd2DataService _obd2Service;

    public MainWindowViewModel()
    {
        _obd2Service = new Obd2DataService();
        _obd2Service.PropertyChanged += OnObd2DataChanged;
    }

    public VehicleData VehicleData => _obd2Service.VehicleData;
    public ChartData ChartData => _obd2Service.ChartData;
    public bool IsConnected => _obd2Service.IsConnected;
    public string ConnectedCarName => "Porsche";

    private void OnObd2DataChanged(object? sender, System.ComponentModel.PropertyChangedEventArgs e)
    {
        if (e.PropertyName == nameof(Obd2DataService.VehicleData))
        {
            OnPropertyChanged(nameof(VehicleData));
        }
        else if (e.PropertyName == nameof(Obd2DataService.ChartData))
        {
            OnPropertyChanged(nameof(ChartData));
        }
        else if (e.PropertyName == nameof(Obd2DataService.IsConnected))
        {
            OnPropertyChanged(nameof(IsConnected));
        }
    }

    public override void Dispose()
    {
        _obd2Service?.Dispose();
        base.Dispose();
    }
}