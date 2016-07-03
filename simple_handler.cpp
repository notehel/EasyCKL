#include "simple_handler.h"

#include <sstream>
#include <string>

#include "include/base/cef_bind.h"
#include "include/cef_app.h"
#include "include/wrapper/cef_closure_task.h"
#include "include/wrapper/cef_helpers.h"
#include "include/cef_parser.h"

CefString GetDataURI(const std::string& data,
	const std::wstring& mime_type) {
	return CefString(L"data:" + mime_type + L";base64," +
		CefURIEncode(CefBase64Encode(data.data(), data.size()), false).ToWString());
}

CefString GetDataURI(const std::string& data,
	const std::wstring& mime_type, const std::wstring& charset) {
	//UTF-8 GB
	return CefString(L"data:" + mime_type + L";charset=" + charset + L";base64," +
		CefURIEncode(CefBase64Encode(data.data(), data.size()), false).ToWString());
}

SimpleHandler::SimpleHandler(DWORD id, LPBROWSER_CALLBACKS _callbacks) : callbacks({ 0 }), is_closing_(false), g_id(id), userData(0), lasterror(0), flags(0) {
	SIZE_T length = _callbacks->cbSzie;
	if (length > sizeof(BROWSER_CALLBACKS))
		length = sizeof(BROWSER_CALLBACKS);
	memcpy(&callbacks, _callbacks, length);
}

SimpleHandler::SimpleHandler(DWORD id, Chrome_CallBack_BrowserCreated callback, Chrome_CallBack_ChUrl churl,
	Chrome_CallBack_NewWindow nwin, Chrome_CallBack_Download down, Chrome_CallBack_ChState chstate,
	Chrome_CallBack_JSDialog jsdialog, Chrome_CallBack_Error error, Chrome_CallBack_RButtonDown rbuttondown,
	Chrome_CallBack_ChTitle chtitle, Chrome_CallBack_CanLoadUrl canloadurl)

	: callbacks({ 0 }), is_closing_(false), g_id(id), userData(0), lasterror(0), flags(0) {

	//���� CallBack ����ָ��
	callbacks.created_callback = callback;
	callbacks.churl_callback = churl;
	callbacks.newwindow_callback = nwin;
	callbacks.download_callback = down;
	callbacks.chstate_callback = chstate;
	callbacks.jsdialog_callback = jsdialog;
	callbacks.error_callback = error;
	callbacks.rbuttondown_callback = rbuttondown;
	callbacks.chtitle_callback = chtitle;
	callbacks.canloadurl_callback = canloadurl;
	//referer_string = std::wstring(L"");
}

SimpleHandler::~SimpleHandler() {}

void SimpleHandler::OnAfterCreated(CefRefPtr<CefBrowser> browser) {
	CEF_REQUIRE_UI_THREAD();

	if (!g_browser) {
		g_browser = browser.get();
		g_browser->AddRef();
		if (callbacks.created_callback) {
			callbacks.created_callback(g_id, this);
		}
	}

	// Add to the list of existing browsers.
	browser_list_.push_back(browser);
}

bool SimpleHandler::DoClose(CefRefPtr<CefBrowser> browser) {
	CEF_REQUIRE_UI_THREAD();

	CefCookieManager::GetGlobalManager(NULL)->FlushStore(NULL);

	// Closing the main window requires special handling. See the DoClose()
	// documentation in the CEF header for a detailed destription of this
	// process.
	if (browser_list_.size() == 1) {
		// Set a flag to indicate that the window close should be allowed.
		is_closing_ = true;
	}

	// Allow the close. For windowed browsers this will result in the OS close
	// event being sent.
	return false;
}

void SimpleHandler::OnBeforeClose(CefRefPtr<CefBrowser> browser) {
	CEF_REQUIRE_UI_THREAD();

	// Remove from the list of existing browsers.
	BrowserList::iterator bit = browser_list_.begin();
	for (; bit != browser_list_.end(); ++bit) {
		if ((*bit)->IsSame(browser)) {
			browser_list_.erase(bit);
			break;
		}
	}

	if (browser_list_.empty()) {
		if (g_browser)
			g_browser->Release();
		// All browser windows have closed. Quit the application message loop.
		//CefQuitMessageLoop();
	}
}

void SimpleHandler::OnLoadError(CefRefPtr<CefBrowser> browser,
	CefRefPtr<CefFrame> frame,
	ErrorCode errorCode,
	const CefString& errorText,
	const CefString& failedUrl) {
	CEF_REQUIRE_UI_THREAD();

	// Don't display an error for downloaded files.
	if (errorCode == ERR_ABORTED)
		return;

	if (!browser->IsSame(g_browser)) return;

	if (frame->IsMain()) lasterror |= BROWSER_LASTERROR_LOADERROR;
	else lasterror |= BROWSER_LASTERROR_LOADRESERROR;

	if (callbacks.error_callback) {
		callbacks.error_callback(g_id, failedUrl.ToWString().c_str(), FALSE);
		return;
	}

	// Display a load error message.
	std::stringstream s;
	s << "<html><head><meta http-equiv=\"content-type\" content=\"text/html;charset=gb2312\"></head><body bgcolor=\"white\">"
		"<h2>Failed to open this page" <<
		"</h2><p>Error: " << errorCode <<
		"</p><p>Url: " << std::string(failedUrl) <<
		"</p></body></html>";
	frame->LoadURL(GetDataURI(s.str(), L"text/html"));
}

void SimpleHandler::CloseAllBrowsers(bool force_close) {
	if (!CefCurrentlyOn(TID_UI)) {
		// Execute on the UI thread.
		CefPostTask(TID_UI,
			base::Bind(&SimpleHandler::CloseAllBrowsers, this, force_close));
		return;
	}

	if (browser_list_.empty())
		return;

	for (auto it = browser_list_.begin(); it != browser_list_.end(); ++it)
		(*it)->GetHost()->CloseBrowser(force_close);
}