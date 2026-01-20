using CommunityToolkit.Mvvm.ComponentModel;

namespace CarMonitor.ViewModels;

public partial class ViewModelBase : ObservableObject
{
    public virtual void Dispose() { }
}