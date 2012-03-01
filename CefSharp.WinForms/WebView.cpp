#include "WebView.h"

using namespace System::Drawing;

namespace CefSharp
{
namespace WinForms
{
    void WebView::Initialize(String^ address, BrowserSettings^ settings)
    {
        if (!CEF::IsInitialized &&
            !CEF::Initialize(gcnew Settings))
        {
            throw gcnew InvalidOperationException("CEF::Initialize() failed");
        }

        _initialized = gcnew ManualResetEvent(false);
        _settings = settings;

        _browserCore = gcnew BrowserCore(address);
        _scriptCore = new ScriptCore();
    }

    void WebView::WaitForInitialized()
    {
        if (IsInitialized)
        {
            return;
        }

        // TODO: risk of infinite lock
        _initialized->WaitOne();
    }

    void WebView::OnHandleCreated(EventArgs^ e)
    {
        if (DesignMode == false) 
        {
            _clientAdapter = new ClientAdapter(this);

            CefWindowInfo window;
            CefRefPtr<ClientAdapter> ptr = _clientAdapter.get();
            CefString url = toNative(_browserCore->Address);

            HWND hWnd = static_cast<HWND>(Handle.ToPointer());
            RECT rect;
            GetClientRect(hWnd, &rect);
            window.SetAsChild(hWnd, rect);

            CefBrowser::CreateBrowser(window, static_cast<CefRefPtr<CefClient>>(ptr),
                url, *_settings->_browserSettings);
        }
    }

    void WebView::OnSizeChanged(EventArgs^ e)
    {
        if (IsInitialized && !DesignMode)
        {
            HWND hWnd = static_cast<HWND>(Handle.ToPointer());
            RECT rect;
            GetClientRect(hWnd, &rect);
            HDWP hdwp = BeginDeferWindowPos(1);

            HWND browserHwnd = _clientAdapter->GetBrowserHwnd();
            hdwp = DeferWindowPos(hdwp, browserHwnd, NULL, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, SWP_NOZORDER);
            EndDeferWindowPos(hdwp);
        }
    }

    void WebView::OnGotFocus(EventArgs^ e)
    {
        if (IsInitialized && !DesignMode)
        {
            _clientAdapter->GetCefBrowser()->SetFocus(true);
        }
    }

    void WebView::OnInitialized()
    {
        BeginInvoke(gcnew Action<EventArgs^>(this, &WebView::OnSizeChanged), EventArgs::Empty);
        _initialized->Set();
    }

    void WebView::Load(String^ url)
    {
        WaitForInitialized();
        _browserCore->OnLoad();
        _clientAdapter->GetCefBrowser()->GetMainFrame()->LoadURL(toNative(url));
    }

    void WebView::Stop()
    {
        WaitForInitialized();
        _clientAdapter->GetCefBrowser()->StopLoad();

    }

    void WebView::Back()
    {
        WaitForInitialized();
        _clientAdapter->GetCefBrowser()->GoBack();
    }

    void WebView::Forward()
    {
        WaitForInitialized();
        _clientAdapter->GetCefBrowser()->GoForward();
    }

    void WebView::Reload()
    {
        Reload(false);
    }

    void WebView::Reload(bool ignoreCache)
    {
        WaitForInitialized();
        if (ignoreCache)
        {
            _clientAdapter->GetCefBrowser()->ReloadIgnoreCache();
        }
        else
        {
            _clientAdapter->GetCefBrowser()->Reload();
        }
    }

    void WebView::Print()
    {
        WaitForInitialized();
        _clientAdapter->GetCefBrowser()->GetMainFrame()->Print();
    }

    void WebView::ExecuteScript(String^ script)
    {
        WaitForInitialized();

        CefRefPtr<CefBrowser> browser = _clientAdapter->GetCefBrowser();
        CefRefPtr<CefFrame> frame = browser->GetMainFrame();

        _scriptCore->Execute(frame, toNative(script));
    }

    Object^ WebView::EvaluateScript(String^ script)
    {
        return EvaluateScript(script, TimeSpan::MaxValue);
    }

    Object^ WebView::EvaluateScript(String^ script, TimeSpan timeout)
    {
	    WaitForInitialized();

        CefRefPtr<CefBrowser> browser = _clientAdapter->GetCefBrowser();
        CefRefPtr<CefFrame> frame = browser->GetMainFrame();

        return _scriptCore->Evaluate(frame, toNative(script), timeout.TotalMilliseconds);
    }

    void WebView::SetNavState(bool isLoading, bool canGoBack, bool canGoForward)
    {
        _browserCore->SetNavState(isLoading, canGoBack, canGoForward);
    }

    void WebView::RaiseConsoleMessage(String^ message, String^ source, int line)
    {
        ConsoleMessage(this, gcnew ConsoleMessageEventArgs(message, source, line));
    }

    void WebView::OnFrameLoadStart()
    {
        _browserCore->OnFrameLoadStart();
    }

    void WebView::OnFrameLoadEnd()
    {
        _browserCore->OnFrameLoadEnd();
    }
}}