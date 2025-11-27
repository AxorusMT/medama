// medama_directory_organizer.cpp
//
// Build with wxWidgets 3.x, e.g. (roughly):
// g++ medama_directory_organizer.cpp `wx-config --cxxflags --libs std,aui` -o medama
//
// Make sure you have the wxWidgets headers/libs set up properly.

#include <wx/wx.h>
#include <wx/listctrl.h>
#include <wx/filedlg.h>
#include <wx/filename.h>
#include <wx/datetime.h>
#include <wx/textfile.h>
#include <wx/scrolwin.h>
#include <wx/simplebook.h>
#include <wx/statline.h>
#include <map>
#include <vector>
#include <algorithm>

struct FileInfo {
    wxString   path;
    wxString   name;
    wxULongLong size;
    wxDateTime modified;
};

enum class Strategy {
    ByType = 0,
    ByDate,
    BySize,
    ByExtension
};

class MainFrame : public wxFrame {
public:
    MainFrame();

private:
    // State
    std::vector<FileInfo> m_files;
    std::map<wxString, std::vector<FileInfo>> m_organized;
    Strategy m_strategy = Strategy::ByType;

    // UI
    wxPanel*      m_mainPanel = nullptr;
    wxPanel*      m_headerPanel = nullptr;
    wxPanel*      m_settingsPanel = nullptr;
    wxSimplebook* m_book = nullptr;          // switches between "welcome", "selected", "organized"

    wxPanel*      m_pageWelcome = nullptr;
    wxPanel*      m_pageSelected = nullptr;
    wxPanel*      m_pageOrganized = nullptr;

    wxListCtrl*   m_selectedList = nullptr;
    wxScrolledWindow* m_organizedScroll = nullptr;
    wxStaticText* m_organizedSummary = nullptr;

    wxRadioBox*   m_strategyRadio = nullptr;

    bool          m_settingsVisible = false;

    // Helpers
    void BuildUI();
    void BuildHeader(wxBoxSizer* rootSizer);
    void BuildSettings(wxBoxSizer* rootSizer);
    void BuildPages(wxBoxSizer* rootSizer);

    void RebuildSelectedList();
    void RebuildOrganizedView();

    // Logic from your React code
    wxString GetCategoryForFile(const FileInfo& file) const;
    wxString GetSizeCategory(wxULongLong bytes) const;
    wxString GetDateCategory(const wxDateTime& dt) const;
    wxString FormatFileSize(wxULongLong bytes) const;

    // Events
    void OnToggleSettings(wxCommandEvent& evt);
    void OnSelectFiles(wxCommandEvent& evt);
    void OnClearFiles(wxCommandEvent& evt);
    void OnOrganize(wxCommandEvent& evt);
    void OnExportPlan(wxCommandEvent& evt);
    void OnStrategyChanged(wxCommandEvent& evt);

    wxDECLARE_EVENT_TABLE();
};

enum {
    ID_BTN_SETTINGS = wxID_HIGHEST + 1,
    ID_BTN_SELECT_FILES,
    ID_BTN_CLEAR_FILES,
    ID_BTN_ORGANIZE,
    ID_BTN_EXPORT_PLAN,
    ID_STRATEGY_RADIO
};

wxBEGIN_EVENT_TABLE(MainFrame, wxFrame)
    EVT_BUTTON(ID_BTN_SETTINGS,      MainFrame::OnToggleSettings)
    EVT_BUTTON(ID_BTN_SELECT_FILES,  MainFrame::OnSelectFiles)
    EVT_BUTTON(ID_BTN_CLEAR_FILES,   MainFrame::OnClearFiles)
    EVT_BUTTON(ID_BTN_ORGANIZE,      MainFrame::OnOrganize)
    EVT_BUTTON(ID_BTN_EXPORT_PLAN,   MainFrame::OnExportPlan)
    EVT_RADIOBOX(ID_STRATEGY_RADIO,  MainFrame::OnStrategyChanged)
wxEND_EVENT_TABLE()

class MedamaApp : public wxApp {
public:
    bool OnInit() override {
        if (!wxApp::OnInit())
            return false;

        MainFrame* frame = new MainFrame();
        frame->Show(true);
        return true;
    }
};

wxIMPLEMENT_APP(MedamaApp);

// ---------------------- MainFrame implementation ----------------------

MainFrame::MainFrame()
    : wxFrame(nullptr, wxID_ANY, "Medama - Intelligent Directory Organizer",
              wxDefaultPosition, wxSize(900, 600))
{
    SetBackgroundColour(wxColour(15, 15, 30));
    BuildUI();
    Centre();
}

void MainFrame::BuildUI()
{
    m_mainPanel = new wxPanel(this);
    m_mainPanel->SetBackgroundColour(wxColour(15, 15, 30));

    auto* rootSizer = new wxBoxSizer(wxVERTICAL);

    BuildHeader(rootSizer);
    BuildSettings(rootSizer);
    BuildPages(rootSizer);

    m_mainPanel->SetSizer(rootSizer);
    rootSizer->SetSizeHints(this);
}

void MainFrame::BuildHeader(wxBoxSizer* rootSizer)
{
    m_headerPanel = new wxPanel(m_mainPanel);
    m_headerPanel->SetBackgroundColour(wxColour(10, 10, 25));

    auto* sizer = new wxBoxSizer(wxHORIZONTAL);

    // Left: icon + title
    auto* leftSizer = new wxBoxSizer(wxHORIZONTAL);

    // We just use a coloured box as a "logo" placeholder
    wxPanel* logoPanel = new wxPanel(m_headerPanel, wxID_ANY, wxDefaultPosition, wxSize(40, 40));
    logoPanel->SetBackgroundColour(wxColour(120, 70, 200));
    leftSizer->Add(logoPanel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);

    auto* titleSizer = new wxBoxSizer(wxVERTICAL);
    auto* title = new wxStaticText(m_headerPanel, wxID_ANY, "Medama");
    auto* subtitle = new wxStaticText(m_headerPanel, wxID_ANY, "Intelligent Directory Organizer");

    title->SetForegroundColour(*wxWHITE);
    title->SetFont(wxFontInfo(14).Bold());

    subtitle->SetForegroundColour(wxColour(180, 140, 255));
    subtitle->SetFont(wxFontInfo(9));

    titleSizer->Add(title, 0, wxBOTTOM, 2);
    titleSizer->Add(subtitle, 0);

    leftSizer->Add(titleSizer, 0, wxALIGN_CENTER_VERTICAL);

    sizer->Add(leftSizer, 1, wxALL | wxEXPAND, 10);

    // Right: Settings button
    auto* btnSettings = new wxButton(m_headerPanel, ID_BTN_SETTINGS, "Settings");
    sizer->Add(btnSettings, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);

    m_headerPanel->SetSizerAndFit(sizer);

    rootSizer->Add(m_headerPanel, 0, wxEXPAND);
    rootSizer->Add(new wxStaticLine(m_mainPanel), 0, wxEXPAND);
}

void MainFrame::BuildSettings(wxBoxSizer* rootSizer)
{
    m_settingsPanel = new wxPanel(m_mainPanel);
    m_settingsPanel->SetBackgroundColour(wxColour(25, 25, 50));

    auto* sizer = new wxBoxSizer(wxVERTICAL);

    auto* titleSizer = new wxBoxSizer(wxHORIZONTAL);
    auto* icon = new wxStaticText(m_settingsPanel, wxID_ANY, "⚙");
    icon->SetForegroundColour(wxColour(200, 200, 255));
    icon->SetFont(wxFontInfo(12));

    auto* label = new wxStaticText(m_settingsPanel, wxID_ANY, "Organization Strategy");
    label->SetForegroundColour(*wxWHITE);
    label->SetFont(wxFontInfo(11).Bold());

    titleSizer->Add(icon, 0, wxRIGHT | wxALIGN_CENTER_VERTICAL, 5);
    titleSizer->Add(label, 0, wxALIGN_CENTER_VERTICAL);

    sizer->Add(titleSizer, 0, wxALL, 10);

    wxString choices[] = {
        "By File Type",
        "By Date Modified",
        "By File Size",
        "By Extension"
    };

    m_strategyRadio = new wxRadioBox(
        m_settingsPanel, ID_STRATEGY_RADIO, "Strategy",
        wxDefaultPosition, wxDefaultSize,
        WXSIZEOF(choices), choices, 2, wxRA_SPECIFY_COLS
    );
    m_strategyRadio->SetSelection(0);

    sizer->Add(m_strategyRadio, 0, wxALL, 10);

    m_settingsPanel->SetSizer(sizer);

    m_settingsVisible = false;
    m_settingsPanel->Hide();

    rootSizer->Add(m_settingsPanel, 0, wxEXPAND);
}

void MainFrame::BuildPages(wxBoxSizer* rootSizer)
{
    m_book = new wxSimplebook(m_mainPanel);
    m_book->SetBackgroundColour(wxColour(15, 15, 30));

    // Page 0: Welcome / select files
    m_pageWelcome = new wxPanel(m_book);
    {
        m_pageWelcome->SetBackgroundColour(wxColour(15, 15, 30));
        auto* sizer = new wxBoxSizer(wxVERTICAL);

        auto* info = new wxStaticText(
            m_pageWelcome, wxID_ANY,
            "Select Files to Organize"
        );
        info->SetForegroundColour(*wxWHITE);
        info->SetFont(wxFontInfo(18).Bold());
        sizer->Add(info, 0, wxALIGN_CENTER_HORIZONTAL | wxTOP, 40);

        auto* desc = new wxStaticText(
            m_pageWelcome, wxID_ANY,
            "Choose multiple files and Medama will intelligently organize them for you."
        );
        desc->SetForegroundColour(wxColour(180, 140, 255));
        desc->SetFont(wxFontInfo(10));
        sizer->Add(desc, 0, wxALIGN_CENTER_HORIZONTAL | wxTOP | wxLEFT | wxRIGHT, 10);

        auto* btnSelect = new wxButton(
            m_pageWelcome, ID_BTN_SELECT_FILES,
            "Choose Files..."
        );
        sizer->Add(btnSelect, 0, wxALIGN_CENTER_HORIZONTAL | wxTOP, 30);

        m_pageWelcome->SetSizer(sizer);
    }

    // Page 1: Selected files (before organizing)
    m_pageSelected = new wxPanel(m_book);
    {
        m_pageSelected->SetBackgroundColour(wxColour(15, 15, 30));
        auto* sizer = new wxBoxSizer(wxVERTICAL);

        auto* headerSizer = new wxBoxSizer(wxHORIZONTAL);
        auto* title = new wxStaticText(m_pageSelected, wxID_ANY, "Selected Files");
        title->SetForegroundColour(*wxWHITE);
        title->SetFont(wxFontInfo(14).Bold());
        headerSizer->Add(title, 1, wxALIGN_CENTER_VERTICAL);

        auto* btnClear = new wxButton(m_pageSelected, ID_BTN_CLEAR_FILES, "Clear");
        auto* btnOrganize = new wxButton(m_pageSelected, ID_BTN_ORGANIZE, "Organize");
        headerSizer->Add(btnClear, 0, wxLEFT, 5);
        headerSizer->Add(btnOrganize, 0, wxLEFT, 5);

        sizer->Add(headerSizer, 0, wxALL | wxEXPAND, 10);

        m_selectedList = new wxListCtrl(
            m_pageSelected, wxID_ANY, wxDefaultPosition, wxDefaultSize,
            wxLC_REPORT | wxLC_SINGLE_SEL
        );

        m_selectedList->InsertColumn(0, "Name", wxLIST_FORMAT_LEFT, 400);
        m_selectedList->InsertColumn(1, "Size", wxLIST_FORMAT_LEFT, 120);
        m_selectedList->InsertColumn(2, "Modified", wxLIST_FORMAT_LEFT, 200);

        sizer->Add(m_selectedList, 1, wxALL | wxEXPAND, 10);

        m_pageSelected->SetSizer(sizer);
    }

    // Page 2: Organized view
    m_pageOrganized = new wxPanel(m_book);
    {
        m_pageOrganized->SetBackgroundColour(wxColour(15, 15, 30));
        auto* vbox = new wxBoxSizer(wxVERTICAL);

        auto* headerSizer = new wxBoxSizer(wxHORIZONTAL);

        m_organizedSummary = new wxStaticText(m_pageOrganized, wxID_ANY, "");
        m_organizedSummary->SetForegroundColour(wxColour(180, 140, 255));
        m_organizedSummary->SetFont(wxFontInfo(10));

        headerSizer->Add(m_organizedSummary, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);

        auto* btnExport = new wxButton(m_pageOrganized, ID_BTN_EXPORT_PLAN, "Export Plan");
        auto* btnNew = new wxButton(m_pageOrganized, ID_BTN_CLEAR_FILES, "New Organization");
        headerSizer->Add(btnExport, 0, wxLEFT, 5);
        headerSizer->Add(btnNew, 0, wxLEFT, 5);

        vbox->Add(headerSizer, 0, wxALL | wxEXPAND, 10);

        m_organizedScroll = new wxScrolledWindow(m_pageOrganized, wxID_ANY,
                                                 wxDefaultPosition, wxDefaultSize,
                                                 wxVSCROLL);
        m_organizedScroll->SetScrollRate(0, 10);
        m_organizedScroll->SetBackgroundColour(wxColour(10, 10, 25));

        vbox->Add(m_organizedScroll, 1, wxALL | wxEXPAND, 10);

        m_pageOrganized->SetSizer(vbox);
    }

    m_book->AddPage(m_pageWelcome, "Welcome");
    m_book->AddPage(m_pageSelected, "Selected");
    m_book->AddPage(m_pageOrganized, "Organized");

    m_book->SetSelection(0);

    rootSizer->Add(m_book, 1, wxEXPAND | wxALL, 10);
}

// -------------------- Helper logic (ported from React) --------------------

wxString MainFrame::FormatFileSize(wxULongLong bytes) const
{
    double b = static_cast<double>(bytes.GetValue());

    if (b < 1024.0)
        return wxString::Format("%.0f B", b);
    if (b < 1024.0 * 1024.0)
        return wxString::Format("%.1f KB", b / 1024.0);
    if (b < 1024.0 * 1024.0 * 1024.0)
        return wxString::Format("%.1f MB", b / (1024.0 * 1024.0));
    return wxString::Format("%.1f GB", b / (1024.0 * 1024.0 * 1024.0));
}

wxString MainFrame::GetSizeCategory(wxULongLong bytes) const
{
    double b = static_cast<double>(bytes.GetValue());

    if (b < 1024.0 * 100.0)              return "Tiny (< 100KB)";
    if (b < 1024.0 * 1024.0)             return "Small (< 1MB)";
    if (b < 1024.0 * 1024.0 * 10.0)      return "Medium (< 10MB)";
    if (b < 1024.0 * 1024.0 * 100.0)     return "Large (< 100MB)";
    return "Very Large (> 100MB)";
}

wxString MainFrame::GetDateCategory(const wxDateTime& dt) const
{
    wxDateTime now = wxDateTime::Now();
    wxTimeSpan diff = now - dt;
    long diffDays = diff.GetDays();

    if (diffDays == 0)         return "Today";
    if (diffDays == 1)         return "Yesterday";
    if (diffDays < 7)          return "This Week";
    if (diffDays < 30)         return "This Month";
    if (diffDays < 90)         return "Last 3 Months";
    if (diffDays < 365)        return "This Year";
    return "Older";
}

wxString MainFrame::GetCategoryForFile(const FileInfo& file) const
{
    wxString ext = wxFileName(file.name).GetExt().Lower();

    // Images
    if (ext == "jpg" || ext == "jpeg" || ext == "png" || ext == "gif" ||
        ext == "bmp" || ext == "svg"  || ext == "webp" || ext == "ico")
        return "Images";

    // Audio
    if (ext == "mp3" || ext == "wav" || ext == "flac" || ext == "aac" ||
        ext == "m4a" || ext == "ogg" || ext == "wma")
        return "Audio";

    // Video
    if (ext == "mp4" || ext == "avi" || ext == "mkv" || ext == "mov" ||
        ext == "wmv" || ext == "flv" || ext == "webm" || ext == "mpeg")
        return "Videos";

    // Code
    if (ext == "js" || ext == "jsx" || ext == "ts"   || ext == "tsx" ||
        ext == "py" || ext == "java"|| ext == "cpp" || ext == "c"   ||
        ext == "h"  || ext == "cs"  || ext == "php" || ext == "rb"  ||
        ext == "go" || ext == "rs"  || ext == "swift" ||
        ext == "html" || ext == "css" || ext == "json" || ext == "xml")
        return "Code";

    // Archives
    if (ext == "zip" || ext == "rar" || ext == "7z" || ext == "tar" ||
        ext == "gz"  || ext == "bz2" || ext == "xz")
        return "Archives";

    // Documents
    if (ext == "txt" || ext == "doc" || ext == "docx" || ext == "pdf" ||
        ext == "rtf" || ext == "odt" || ext == "pages")
        return "Documents";

    // Spreadsheets
    if (ext == "xls" || ext == "xlsx" || ext == "csv" || ext == "ods" || ext == "numbers")
        return "Spreadsheets";

    // Presentations
    if (ext == "ppt" || ext == "pptx" || ext == "odp" || ext == "key")
        return "Presentations";

    return "Other";
}

// ----------------------------- UI updates -----------------------------

void MainFrame::RebuildSelectedList()
{
    m_selectedList->DeleteAllItems();

    for (size_t i = 0; i < m_files.size(); ++i) {
        const FileInfo& f = m_files[i];
        long index = m_selectedList->InsertItem(i, f.name);
        m_selectedList->SetItem(index, 1, FormatFileSize(f.size));
        m_selectedList->SetItem(index, 2, f.modified.FormatISOCombined(' '));
    }
}

void MainFrame::RebuildOrganizedView()
{
    m_organizedScroll->DestroyChildren();

    auto* vbox = new wxBoxSizer(wxVERTICAL);

    // Sort categories alphabetically
    std::vector<wxString> keys;
    keys.reserve(m_organized.size());
    for (const auto& kv : m_organized)
        keys.push_back(kv.first);
    std::sort(keys.begin(), keys.end());

    for (const wxString& key : keys) {
        const auto& files = m_organized[key];

        auto* box = new wxStaticBox(m_organizedScroll, wxID_ANY,
                                    wxString::Format("%s (%zu files)", key, files.size()));
        box->SetForegroundColour(*wxWHITE);
        auto* boxSizer = new wxStaticBoxSizer(box, wxVERTICAL);

        for (const auto& f : files) {
            auto* row = new wxBoxSizer(wxHORIZONTAL);

            auto* nameText = new wxStaticText(box, wxID_ANY, f.name);
            nameText->SetForegroundColour(*wxWHITE);

            auto* sizeText = new wxStaticText(box, wxID_ANY, FormatFileSize(f.size));
            sizeText->SetForegroundColour(wxColour(180, 140, 255));

            row->Add(nameText, 1, wxRIGHT, 10);
            row->Add(sizeText, 0);

            boxSizer->Add(row, 0, wxTOP | wxBOTTOM | wxEXPAND, 2);
        }

        vbox->Add(boxSizer, 0, wxALL | wxEXPAND, 5);
    }

    m_organizedScroll->SetSizer(vbox);
    vbox->FitInside(m_organizedScroll);
    m_organizedScroll->Layout();
}

// ----------------------------- Events --------------------------------

void MainFrame::OnToggleSettings(wxCommandEvent& WXUNUSED(evt))
{
    m_settingsVisible = !m_settingsVisible;
    m_settingsPanel->Show(m_settingsVisible);
    m_mainPanel->Layout();
}

void MainFrame::OnSelectFiles(wxCommandEvent& WXUNUSED(evt))
{
    wxFileDialog dlg(
        this, "Select files",
        wxEmptyString, wxEmptyString,
        "All files (*.*)|*.*",
        wxFD_OPEN | wxFD_MULTIPLE | wxFD_FILE_MUST_EXIST
    );

    if (dlg.ShowModal() != wxID_OK)
        return;

    wxArrayString paths;
    dlg.GetPaths(paths);

    m_files.clear();
    m_organized.clear();

    for (unsigned i = 0; i < paths.size(); ++i) {
        FileInfo fi;
        fi.path = paths[i];
        wxFileName fn(paths[i]);
        fi.name = fn.GetFullName();
        fi.size = fn.GetSize();

        wxDateTime atime, mtime, ctime;
        fn.GetTimes(&atime, &mtime, &ctime);
        fi.modified = mtime.IsValid() ? mtime : wxDateTime::Now();

        m_files.push_back(fi);
    }

    RebuildSelectedList();
    m_book->SetSelection(1); // Selected files page
}

void MainFrame::OnClearFiles(wxCommandEvent& WXUNUSED(evt))
{
    m_files.clear();
    m_organized.clear();

    if (m_selectedList)
        m_selectedList->DeleteAllItems();

    if (m_organizedScroll)
        m_organizedScroll->DestroyChildren();

    m_organizedSummary->SetLabel("");
    m_book->SetSelection(0); // Back to welcome
}

void MainFrame::OnOrganize(wxCommandEvent& WXUNUSED(evt))
{
    if (m_files.empty())
        return;

    m_organized.clear();

    for (const auto& f : m_files) {
        wxString category;

        switch (m_strategy) {
        case Strategy::ByType:
            category = GetCategoryForFile(f);
            break;
        case Strategy::ByDate:
            category = GetDateCategory(f.modified);
            break;
        case Strategy::BySize:
            category = GetSizeCategory(f.size);
            break;
        case Strategy::ByExtension: {
            wxString ext = wxFileName(f.name).GetExt().Lower();
            if (!ext.IsEmpty())
                category = "." + ext;
            else
                category = "No Extension";
            break;
        }
        }

        m_organized[category].push_back(f);
    }

    wxString strategyText;
    switch (m_strategy) {
    case Strategy::ByType:      strategyText = "By File Type"; break;
    case Strategy::ByDate:      strategyText = "By Date Modified"; break;
    case Strategy::BySize:      strategyText = "By File Size"; break;
    case Strategy::ByExtension: strategyText = "By Extension"; break;
    }

    m_organizedSummary->SetLabel(
        wxString::Format("%zu categories • %zu files organized (%s)",
                         m_organized.size(), m_files.size(), strategyText)
    );

    RebuildOrganizedView();
    m_book->SetSelection(2); // Organized page
}

void MainFrame::OnExportPlan(wxCommandEvent& WXUNUSED(evt))
{
    if (m_organized.empty())
        return;

    wxFileDialog dlg(
        this, "Save organization plan",
        wxEmptyString, "organization-plan.txt",
        "Text files (*.txt)|*.txt|All files (*.*)|*.*",
        wxFD_SAVE | wxFD_OVERWRITE_PROMPT
    );

    if (dlg.ShowModal() != wxID_OK)
        return;

    wxString filename = dlg.GetPath();

    wxTextFile tf;
    if (wxFileExists(filename)) {
        tf.Open(filename);
        tf.Clear();
    } else {
        tf.Create(filename);
    }

    wxString strategyText;
    switch (m_strategy) {
    case Strategy::ByType:      strategyText = "By File Type"; break;
    case Strategy::ByDate:      strategyText = "By Date Modified"; break;
    case Strategy::BySize:      strategyText = "By File Size"; break;
    case Strategy::ByExtension: strategyText = "By Extension"; break;
    }

    tf.AddLine("Directory Organization Plan (" + strategyText + ")");
    tf.AddLine(wxString('=', 60));
    tf.AddLine("");

    std::vector<wxString> keys;
    keys.reserve(m_organized.size());
    for (const auto& kv : m_organized)
        keys.push_back(kv.first);
    std::sort(keys.begin(), keys.end());

    for (const wxString& key : keys) {
        const auto& files = m_organized[key];
        tf.AddLine(wxString::Format("📁 %s/ (%zu files)", key, files.size()));
        for (const auto& f : files) {
            tf.AddLine("   └─ " + f.name + " (" + FormatFileSize(f.size) + ")");
        }
        tf.AddLine("");
    }

    tf.Write();
    tf.Close();

    wxMessageBox("Organization plan exported successfully.", "Medama",
                 wxOK | wxICON_INFORMATION, this);
}

void MainFrame::OnStrategyChanged(wxCommandEvent& evt)
{
    int sel = evt.GetSelection();
    switch (sel) {
    default:
    case 0: m_strategy = Strategy::ByType;      break;
    case 1: m_strategy = Strategy::ByDate;      break;
    case 2: m_strategy = Strategy::BySize;      break;
    case 3: m_strategy = Strategy::ByExtension; break;
    }
}
