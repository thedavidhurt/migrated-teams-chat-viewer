# migrated-teams-chat-viewer

Viewer for migrated teams chat

Latest executable is migrated-teams-chat-viewer-vX.X.zip at https://github.com/thedavidhurt/migrated-teams-chat-viewer/releases/latest

## Usage instructions
### Export migrated Teams chat folder to .csv file
#### By default, Outlook only stores one year of email locally. If you only want the last year's worth of Teams chat, skip to step 16. Otherwise, follow the instructions to make Outlook download the rest of the Teams chat messages before exporting.
1. Right click on the Inbox folder and select Properties... 
2. Under the Synchronization tab, click Filter... 
3. Under the Advanced tab, click Field > All Mail fields > Received 
4. Under Condition, select "on or after" 
5. Under Value, enter "1 year ago" 
6. Click "Add to List", then OK 
7. Click OK on the popup 
8. Click OK on the Properties window 
9. Navigate to File > Account Settings > Account Name and Sync Settings 
10. Drag the slider all the way to the right until it reads "All" 
11. Click Next and then Done 
12. Close Outlook 
13. Open Outlook 
14. Click the "Migrated Teams Chat" folder 
15. Watch the status at the bottom and wait for it to read "This folder is up to date." This could take a while.
##### Start here to only export the last year of Teams chats
16. Navigate to File > Open & Export > Import/Export
17. Select "Export to a file" and click Next 
18. Select Comma Separated Values and click Next 
19. Select "Migrated Teams Chat" and click Next 
20. Choose a file name and location and click Next. (Example: C:\Users\dhurt\Documents\MigratedTeamsChat.csv) 
21. Click Finish
##### If you followed steps 1-15, follow these steps to reset your offline storage settings
22. Right click on the Inbox folder and select Properties... 
23. Under the Synchronization tab, click Filter... 
24. Click Clear All
25. Click OK
26. Click OK on the Properties window
27. Navigate to File > Account Settings > Account Name and Sync Settings 
28. Drag the slider to the left until it reads "1 year" 
29. Click Next and then Done 
30. Close Outlook 
31. Open Outlook and let it re-synchronize using the new settings

### Viewing exported Teams chat
1. Download latest migrated-teams-chat-viewer-vX.X.zip release from https://github.com/thedavidhurt/migrated-teams-chat-viewer/releases/latest
2. Open the .zip file and click Extract All
3. Choose a location and click Extract
4. Open the migrated-teams-chat-viewer-vX.X folder and double-click migrated-teams-chat-viewer.exe to run
5. At the popup, uncheck the "Always ask before opening this file" box and click Run
6. Click Browse to locate the exported .csv file
7. Click Open to read the file

The conversation list in the left pane can be filtered by subject, participant names, or by the results of a global search.
The conversation on the right can be searched for text and zoomed in and out using Ctrl-Mousewheel.

## Building
*  Use Qt Creator to open and build the project: https://www.qt.io/download-qt-installer
*  When opening the project in Qt for the first time, select a MSVC kit
*  Use windeployqt to make the build folder redistributable:
    *  Qt > Projects > Build > Build Steps > Add Build Step > Custom Process Step
        *  Command: %{Qt:QT_INSTALL_BINS}\windeployqt.exe
        *  Arguments: %{CurrentRun:Executable:FileName}
        *  Working Directory: %{CurrentRun:Executable:Path}
