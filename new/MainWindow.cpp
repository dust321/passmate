#include <sstream>

#include "MainWindow.hpp"
#include "Application.hpp"
#include "Storage.hpp"

#include <wx/splitter.h>
#include <wx/sizer.h>

#include <wx/textctrl.h>

using namespace std;

MainWindow::MainWindow()
    : wxFrame(NULL, wxID_ANY, wxT("Passmate"), wxDefaultPosition, wxSize(0, 0))
    , irt_root(NULL, "")
{
    
    // Panels
    wxSplitterWindow *splittermain = new wxSplitterWindow(this,wxID_ANY,wxDefaultPosition,wxDefaultSize,wxSP_3D);
    wxPanel *panelLeft=new wxPanel(splittermain,wxID_ANY,wxDefaultPosition,wxDefaultSize,wxTAB_TRAVERSAL|wxNO_BORDER);
    wxPanel *panelRight=new wxPanel(splittermain,wxID_ANY,wxDefaultPosition,wxDefaultSize,wxTAB_TRAVERSAL|wxNO_BORDER);
    splittermain->SplitVertically(panelLeft, panelRight);
    panelRecord=new wxScrolledWindow(panelRight,wxID_ANY,wxDefaultPosition,wxDefaultSize, wxVSCROLL|wxBORDER_SUNKEN);


    // Widgets
    wxTextCtrl *entryFilter=new wxTextCtrl( panelLeft, wxID_ANY, wxT(""), wxDefaultPosition, wxDefaultSize, 0);

    recordTree=new wxTreeCtrl(panelLeft, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTR_DEFAULT_STYLE/*|wxTR_HIDE_ROOT*/);

    wxButton *buttonAdd=new wxButton(panelLeft, wxID_ANY, _T("Add record"));
    wxButton *buttonSync=new wxButton(panelLeft, wxID_ANY, _T("Sync database"));

    wxButton *buttonRemove=new wxButton(panelRight, wxID_ANY, _T("Remove record"));
    wxButton *buttonRename=new wxButton(panelRight, wxID_ANY, _T("Rename record"));
    wxButton *buttonAddField=new wxButton(panelRight, wxID_ANY, _T("Add field"));
    wxButton *buttonSaveChanges=new wxButton(panelRight, wxID_ANY, _T("Save"));
    wxButton *buttonHistory=new wxButton(panelRight, wxID_ANY, _T("History"));

    // Sizing
    panelLeft->SetMinSize(wxSize(200, 200));
    panelRight->SetMinSize(wxSize(500, 200));
    splittermain->SetMinSize(wxSize(700, 400));
    splittermain->SetMinimumPaneSize(10);
    splittermain->SetSashPosition(250);
    splittermain->SetSashGravity(0.0); // When the main window is resized, resize only right panel.


    // Sizer
    wxBoxSizer *windowSizer = new wxBoxSizer(wxVERTICAL);
    windowSizer->Add(splittermain,1,wxBOTTOM|wxLEFT|wxEXPAND,3);
    SetSizer(windowSizer);
    windowSizer->SetSizeHints(this);

    wxBoxSizer *sizerLeft=new wxBoxSizer(wxVERTICAL);
    sizerLeft->Add(new wxStaticText(panelLeft, wxID_ANY, _T("Filter:")), 0, 0, 0);
    sizerLeft->Add(entryFilter,0,wxEXPAND|wxBOTTOM,5);
    sizerLeft->Add(recordTree,1,wxEXPAND|wxBOTTOM,5);
    sizerLeft->Add(buttonAdd,0, wxEXPAND|wxBOTTOM,5);
    sizerLeft->Add(buttonSync,0, wxEXPAND, 0);
    panelLeft->SetSizer(sizerLeft);

    //sizerLeft->SetSizeHints(panelLeft);

    wxBoxSizer *sizerButtonsRight = new wxBoxSizer(wxHORIZONTAL);
    sizerButtonsRight->Add(buttonRename,1, wxEXPAND|wxRIGHT,5);
    sizerButtonsRight->Add(buttonRemove,1, wxEXPAND|wxRIGHT,5);
    sizerButtonsRight->Add(buttonHistory,1, wxEXPAND|wxRIGHT,5);
    sizerButtonsRight->Add(buttonAddField,1, wxEXPAND|wxRIGHT,5);
    sizerButtonsRight->Add(buttonSaveChanges,1, wxEXPAND,0);

    wxBoxSizer *sizerRight=new wxBoxSizer(wxVERTICAL);
    sizerRight->Add(panelRecord,1,wxALL|wxEXPAND,2);
    sizerRight->Add(sizerButtonsRight,0,wxLEFT|wxRIGHT|wxBOTTOM|wxEXPAND,2);
    panelRight->SetSizer(sizerRight);

    //sizerRight->SetSizeHints(panelRight);


    recordTree->Connect(wxID_ANY, wxEVT_TREE_ITEM_ACTIVATED, wxTreeEventHandler(MainWindow::OnRecordActivated), NULL, this);


    sizerRecord=new wxFlexGridSizer(6, 5, 5); // 6 cols, 5 pixel horizontal and vertical padding
    sizerRecord->AddGrowableCol(1);

    
    UpdateRecordPanel();


    panelRecord->SetSizer(sizerRecord);
    panelRecord->SetScrollRate(0, 10);

    //sizerRecord->SetVirtualSizeHints(panelRecord);

    
    // Do the menu thing
    InitMenu();

    
    UpdateRecordTree();

    Show();

    Connect( wxEVT_SIZE, wxSizeEventHandler( MainWindow::OnSize ) );



    //panelRecord->ShowScrollbars(wxSHOW_SB_ALWAYS, wxSHOW_SB_ALWAYS);
}

/*
void MainWindow::OnFilterUpdate() {

}
*/

void MainWindow::OnSize(wxSizeEvent& event) {
    //printf("resize event\n");    
    //wxSize size = panelRecord->GetBestVirtualSize();
    //panelRecord->SetVirtualSize( size );
    sizerRecord->FitInside(panelRecord);
    panelRecord->Layout();

    Layout();


    int prX, prY, mX, mY;

    panelRecord->GetSize(&prX, &prY);
    panelRecord->GetVirtualSize(&mX, &mY);



    printf("%04i %04i %04i %04i\n", mX, mY, prX, prY);
}

void MainWindow::OnRecordActivated(wxTreeEvent& event) {
    IRTNode *selected = irt_root.FindByItemId(event.GetItem());


    if(selected && selected->path_connected) {
          Storage &st = wxGetApp().GetStorage();

          cur_record = st.GetRecord(selected->full_path);

          cur_record.PrintRecord();

          UpdateRecordPanel();
    }    
}

MainWindow::IRTNode::IRTNode(IRTNode *parent, std::string node_name) {
    this->parent = parent;

    this->node_name=node_name;

    full_path="";
    path_connected = false;

    search_flag = true;
}

MainWindow::IRTNode *MainWindow::IRTNode::GetChildForceCreate(std::string new_node_name) {
    // If we already have it, return that:
    for(IRTNode &node : children) {
        if(node.node_name == new_node_name)
            return &node;
    }

    // Else make a new one, append and return:
    children.push_back(IRTNode(this, new_node_name));

    return &children.back();
}


std::vector<std::string> MainWindow::IRTNode::SplitPath(std::string path) {

    vector<string> ret;
    
    size_t tokEnd = 0;
    while ((tokEnd = path.find("/")) != string::npos) {
        ret.push_back(path.substr(0, tokEnd));
        path.erase(0, tokEnd + 1);
    }
    ret.push_back(path);

    return ret;
}

MainWindow::IRTNode *MainWindow::IRTNode::FindByItemId(const wxTreeItemId &search_id){
    if(item_id == search_id)
        return this;

    for(IRTNode &child : children) {
        IRTNode *result;
        result = child.FindByItemId(search_id);

        if(result)
            return result;
    }

    return NULL;
}

void MainWindow::IRTNode::AppendToTreeCtrl(wxTreeCtrl *tree) {
    if(parent) {
        item_id = tree->AppendItem(parent->item_id, node_name);
    } else {
        item_id = tree->AddRoot(node_name);
    }

    for(MainWindow::IRTNode &child : children) {
        child.AppendToTreeCtrl(tree);
    }
}


void MainWindow::UpdateRecordTree() {
    Storage &st = wxGetApp().GetStorage();

    recordTree->DeleteAllItems();

    //wxTreeItemId root_id=recordTree->AddRoot("passmate db");

    //IRTNode irt_root(NULL, "passmate db");
    irt_root = IRTNode(NULL, "passmate db");

    for (const string &path : st.List()) {
        vector<string> path_split = IRTNode::SplitPath(path);
        int i;

        IRTNode *cur = &irt_root;

        for(i=0;i<path_split.size();i++) {
            cur = cur->GetChildForceCreate(path_split[i]);
        }

        cur->full_path = path;
        cur->path_connected = true;
    }

    irt_root.AppendToTreeCtrl(recordTree);


}

void MainWindow::UpdateRecordPanel() {

    sizerRecord->Clear(true);

    std::map<std::string, std::vector<std::string>> fields = cur_record.GetFields();


    wxStaticText *label;
    wxTextCtrl *entry;
    wxButton *buttonGenerate, *buttonHide, *buttonCopy, *buttonRemove;

    // Path
    label=new wxStaticText(panelRecord, wxID_ANY, std::string("Path:"));
    sizerRecord->Add(label, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 0);   
    label=new wxStaticText(panelRecord, wxID_ANY, cur_record.GetPath());
    sizerRecord->Add(label,0, wxEXPAND|wxTOP|wxBOTTOM, 0);

    sizerRecord->AddSpacer(0);
    sizerRecord->AddSpacer(0);
    sizerRecord->AddSpacer(0);
    sizerRecord->AddSpacer(0);


    // RID
    label=new wxStaticText(panelRecord, wxID_ANY, std::string("RID:"));
    sizerRecord->Add(label, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 0);   
    label=new wxStaticText(panelRecord, wxID_ANY, cur_record.GetId());
    sizerRecord->Add(label,0, wxEXPAND|wxTOP|wxBOTTOM, 0);

    sizerRecord->AddSpacer(0);
    sizerRecord->AddSpacer(0);
    sizerRecord->AddSpacer(0);
    sizerRecord->AddSpacer(0);


    for(auto const &cur : fields) {
        // Make Widgets
        if(cur.second.size() < 1) {
            // TODO: Handle this case this should not be occuring.
            continue;
        }


        label=new wxStaticText(panelRecord, wxID_ANY, cur.first+std::string(":"));
        sizerRecord->Add(label, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 0);   

        entry=new wxTextCtrl( panelRecord, wxID_ANY, cur.second[0], wxDefaultPosition, wxDefaultSize, 0);
        entry->SetMinSize(wxSize(30, 30));
        sizerRecord->Add(entry, 1, wxEXPAND, 0);

        int i;
        for(i=1;i<cur.second.size();i++) {
            // with multi value fields, add the buttons in the last row
            sizerRecord->AddSpacer(0);
            sizerRecord->AddSpacer(0);
            sizerRecord->AddSpacer(0);
            sizerRecord->AddSpacer(0);


            // do not repeat the label in the next line
            sizerRecord->AddSpacer(0);

            // and the second value :)
            entry=new wxTextCtrl( panelRecord, wxID_ANY, cur.second[i], wxDefaultPosition, wxDefaultSize, 0);
            entry->SetMinSize(wxSize(30, 30));
            sizerRecord->Add(entry, 1, wxEXPAND, 0);
        }

        buttonGenerate=new wxButton(panelRecord, wxID_ANY, _T("G"));
        buttonHide=new wxButton(panelRecord, wxID_ANY, _T("H"));
        buttonCopy=new wxButton(panelRecord, wxID_ANY, _T("C"));
        buttonRemove=new wxButton(panelRecord, wxID_ANY, _T("X"));
        buttonGenerate->SetMinSize(wxSize(30, 30));
        buttonHide->SetMinSize(wxSize(30, 30));
        buttonCopy->SetMinSize(wxSize(30, 30));
        buttonRemove->SetMinSize(wxSize(30, 30));
        sizerRecord->Add(buttonGenerate, 0, 0, 0);
        sizerRecord->Add(buttonHide, 0, 0, 0);
        sizerRecord->Add(buttonCopy, 0, 0, 0);
        sizerRecord->Add(buttonRemove, 0, 0, 0);


    }
    


    sizerRecord->ShowItems(true);


    panelRecord->Layout();

    sizerRecord->FitInside(panelRecord);

    //Layout();

    //Show();

    Refresh();
}

void MainWindow::InitMenu() {
    // Setup menu
    wxMenuBar *menubar = new wxMenuBar();
    wxMenu *menuStore, *menuSync, *menuHelp;
    menuStore = new wxMenu();
    menuSync = new wxMenu();
    menuHelp = new wxMenu();
    menuStore->Append(wxID_EXIT, wxT("&Quit"));


    menuHelp->Append(wxID_ANY, wxT("&Documentation"));;
    menuHelp->Append(wxID_ANY, wxT("Visit &Website"));
    

    menubar->Append(menuStore, wxT("&Store"));
    menubar->Append(menuSync, wxT("S&ync"));
    menubar->Append(menuHelp, wxT("&Help"));

    SetMenuBar(menubar);
}

void MainWindow::OnClose(wxCommandEvent& WXUNUSED(event)) {
    // true is to force the frame to close
    Close(true);
}