using System;
using CommunityToolkit.Mvvm.ComponentModel;

namespace CarMonitor.ViewModels;

public class ViewModelBase : ObservableObject, IDisposable
{
    public virtual void Dispose()
    {
        // Override in derived classes if needed
    }
}