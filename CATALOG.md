# 🛡️ PrivatizeWin — Privacy Settings & Tweaks Catalog

> Comprehensive reference of all **354** native privacy, telemetry, ad-blocking, and security settings managed by PrivatizeWin.

[← Back to Main Readme](README.md)

---

## 📊 Categories Overview

| Category | Settings Count | Description |
| :--- | :---: | :--- |
| [AI & Copilot](#ai-copilot) | **14** | Disables Copilot integration, Recall desktop snapshot recordings, Paint Image Creator, and Bing generative AI telemetry. |
| [Activity History & Clipboard](#activity-history-clipboard) | **3** | Blocks cloud synchronization of user activity timeline and clipboard history across devices. |
| [App Permissions & Hardware Access](#app-permissions-hardware-access) | **78** | Restricts background apps from accessing location, camera, microphone, contacts, radios, and diagnostics. |
| [Cortana & Search](#cortana-search) | **17** | Silences cloud search indexing, Bing web suggestions in Start, Cortana assistant, and search telemetry. |
| [Gaming & Xbox](#gaming-xbox) | **1** | Disables Xbox Game DVR background screen capture, telemetry, and broadcast services. |
| [Location & Sensors](#location-sensors) | **7** | Disables Windows location sensors, geo-tracking services, and location history logging. |
| [Lock Screen & Desktop](#lock-screen-desktop) | **3** | Removes Windows Spotlight promotional advertisements, lock screen tips, and sponsored suggestions. |
| [Microsoft Edge](#microsoft-edge) | **102** | Comprehensive hardening of Microsoft Edge: disables shopping trackers, diagnostic reporting, telemetry, and sidebar promotions. |
| [Miscellaneous](#miscellaneous) | **16** | Disables consumer features, diagnostic log collections, CEIP, and background feedback prompts. |
| [Mobile Devices & Phone Link](#mobile-devices-phone-link) | **4** | Disables Phone Link cloud notifications, mobile device sync, and cross-device handoff tracking. |
| [Office & Outlook](#office-outlook) | **22** | Disables connected experience telemetry, data analytics, automated 'New Outlook' migrations, and Office feedback. |
| [Privacy & Tracking](#privacy-tracking) | **25** | Disables Windows advertising ID, typing/ink collection, tailored experiences, and user activity tracking. |
| [Security & Network](#security-network) | **18** | Disables Wi-Fi Sense credential sharing, peer-to-peer telemetry distribution, and legacy insecure protocols. |
| [Synchronization](#synchronization) | **7** | Prevents cloud synchronization of credentials, browser settings, wallpapers, and personalization data. |
| [Taskbar & Start Menu](#taskbar-start-menu) | **7** | Disables Start menu promoted app suggestions, taskbar dynamic MSN news feeds, and 'Meet Now' widgets. |
| [Telemetry & Diagnostics](#telemetry-diagnostics) | **11** | Disables Connected User Experiences and Telemetry (DiagTrack), WAP push routing, and Windows Error Reporting dumps. |
| [Windows Explorer](#windows-explorer) | **5** | Removes cloud advertising banners, OneDrive promotions, and Office 365 recommendations from File Explorer. |
| [Windows Update](#windows-update) | **14** | Configures update deferral, disables peer-to-peer bandwidth harvesting, and stops automated reboots with logged-on users. |

---

## AI & Copilot

*Category contains **14** manageable settings.*

| ID | Setting / Title | Scope | Impact | Description |
| :--- | :--- | :---: | :---: | :--- |
| `AI_COPILOT_USER` | **Disable the Windows Copilot (User)** | `User` | 🟢 Recommended (Safe) | The Windows Copilot is based on ChatGPT from OpenAI and is an extension to the AI in the Microsoft search engine Bing. In order for this AI to provide answers, further system information is transmitted in addition to the user queries. To prevent this, Copilot can be disabled. |
| `AI_COPILOT_MACHINE` | **Disable the Windows Copilot (Machine)** | `Machine` | 🟢 Recommended (Safe) | The Windows Copilot is based on ChatGPT from OpenAI and is an extension to the AI in the Microsoft search engine Bing. In order for this AI to provide answers, further system information is transmitted in addition to the user queries. To prevent this, Copilot can be disabled. |
| `AI_RECALL` | **Disable the provision of Recall functionality to all users** | `Machine` | 🟢 Recommended (Safe) | This setting disables the Recall component for all users on the system. If it was previously enabled, all saved snapshots will be removed when the computer is restarted. |
| `C205` | **Disable the Image Creator in Microsoft Paint** | `Machine` | 🟢 Recommended (Safe) | The Image Creator in Microsoft Paint can create images with the help of artificial intelligence. To do this, appropriate information must be transferred to Microsoft servers. This setting disables this functionality. |
| `C102` | **Disable the Copilot button from the taskbar** | `User` | 🟢 Recommended (Safe) | Removes the Copilot icon from the taskbar so that the search using AI (artificial intelligence) is no longer available. This setting can be used to disable this option. |
| `AI_SEARCH_HIGHLIGHTS` | **Disable Bing Chat eligibility in Windows Copilot** | `User` | 🟢 Recommended (Safe) | Controls whether the user is eligible to use Bing Chat features within the Windows Copilot. When disabled, this prevents access to Bing Chat functionality in Copilot, even if the feature would otherwise be available in the user's region and Windows build. |
| `AI_RECALL_DATA_USER` | **Disable Windows Copilot+ Recall (User)** | `User` | 🟢 Recommended (Safe) | This setting deactivates the new Windows Copilot+ Recall feature. This is a component that constantly creates screenshots, evaluates their content and makes the data available via an application. Both to the user himself and to other applications that have the corresponding authorizations. Disabling the Recall feature is strongly recommended. |
| `AI_RECALL_DATA_MACHINE` | **Disable Windows Copilot+ Recall (Machine)** | `Machine` | 🟢 Recommended (Safe) | This setting deactivates the new Windows Copilot+ Recall feature. This is a component that constantly creates screenshots, evaluates their content and makes the data available via an application. Both to the user himself and to other applications that have the corresponding authorizations. Disabling the Recall feature is strongly recommended. |
| `C206` | **Disable Cocreator in Microsoft Paint** | `Machine` | 🟢 Recommended (Safe) | The cocreator in Microsoft Paint can create images with the help of artificial intelligence. These are created locally with the help of special hardware that must be available (so-called Neural Processing Unit or NPU for short). This setting disables this functionality. |
| `C207` | **Disable AI-powered image fill in Microsoft Paint** | `Machine` | 🟢 Recommended (Safe) | The AI-powered image filling in Microsoft Paint can complement images with the help of artificial intelligence and integrate new objects into existing images. These are created locally with the help of special hardware that must be available (so-called Neural Processing Unit or NPU for short). This setting disables this functionality. |
| `C208` | **Disable Click to Do** | `Machine` | 🟢 Recommended (Safe) | This setting disables Click to Do on Copilot+ PCs. Click to Do can analyze screen content when invoked to suggest actions. |
| `C209` | **Disable the Settings agent** | `Machine` | 🟢 Recommended (Safe) | This setting disables the AI agent in the Windows Settings app that can search for and change settings using an on-device model. |
| `C210` | **Disable AI features in Notepad** | `Machine` | 🟢 Recommended (Safe) | This setting disables Notepad AI features such as Rewrite and related text generation features that may send document text to an online service. |
| `C211` | **Disable AI actions in File Explorer** | `Machine` | 🟢 Recommended (Safe) | This setting removes the AI actions from the File Explorer context menu, so that AI actions such as image editing or document summarization are no longer offered when you right-click a file. |

[↑ Back to Top](#-categories-overview)

---

## Activity History & Clipboard

*Category contains **3** manageable settings.*

| ID | Setting / Title | Scope | Impact | Description |
| :--- | :--- | :---: | :---: | :--- |
| `PRIV_TIMELINE` | **Disable recordings of user activity** | `Machine` | 🟢 Recommended (Safe) | Windows records user activity such as surfing on the Internet and the use of applications in order to be able to create evaluations for the user locally and in the cloud (so-called Microsoft Graph). This includes sensitive information and should be disabled to protect privacy. |
| `A002` | **Disable storing users' activity history** | `Machine` | 🟢 Recommended (Safe) | Windows stores user activities such as surfing on the Internet and the use of applications in order to be able to create evaluations for the user (so-called Microsoft Graph). This includes sensitive information and should be disabled to protect privacy. |
| `A003` | **Disable the submission of user activities to Microsoft** | `Machine` | 🟢 Recommended (Safe) | Windows sends user activities such as surfing the Internet and the use of applications to Microsoft in order to be able to create evaluations for the user in the cloud (so-called Microsoft Graph). This includes sensitive information and should be disabled to protect privacy. |

[↑ Back to Top](#-categories-overview)

---

## App Permissions & Hardware Access

*Category contains **78** manageable settings.*

| ID | Setting / Title | Scope | Impact | Description |
| :--- | :--- | :---: | :---: | :--- |
| `P025_USER` | **Disable app access to device location (User)** | `User` | 🟢 Recommended (Safe) | When this feature is disabled, apps will no longer have access to the location of your computer. Some apps may be restricted in your function or may no longer work at all. |
| `P025_MACHINE` | **Disable app access to device location (Machine)** | `Machine` | 🟢 Recommended (Safe) | When this feature is disabled, apps will no longer have access to the location of your computer. Some apps may be restricted in your function or may no longer work at all. |
| `P023_USER` | **Disable app access to diagnostics information (User)** | `User` | 🟢 Recommended (Safe) | Disabling this function means apps will no longer have access to diagnostic information from your system. These are needed for finding sources of error from the respective manufacturers. You can disable this feature if you don’t wish to permit this. |
| `P023_MACHINE` | **Disable app access to diagnostics information (Machine)** | `Machine` | 🟢 Recommended (Safe) | Disabling this function means apps will no longer have access to diagnostic information from your system. These are needed for finding sources of error from the respective manufacturers. You can disable this feature if you don’t wish to permit this. |
| `P082_USER` | **Deny app access to generative AI (User)** | `User` | 🟢 Recommended (Safe) | This setting denies apps access to Windows text and image generation capabilities. It writes both known capability names used by recent Windows builds. |
| `P082_MACHINE` | **Deny app access to generative AI (Machine)** | `Machine` | 🟢 Recommended (Safe) | This setting denies apps access to Windows text and image generation capabilities. It writes both known capability names used by recent Windows builds. |
| `P084_USER` | **Deny app access to presence sensing (User)** | `User` | 🟢 Recommended (Safe) | This setting denies apps access to human presence sensors used for features such as wake on approach and lock on leave. |
| `P084_MACHINE` | **Deny app access to presence sensing (Machine)** | `Machine` | 🟢 Recommended (Safe) | This setting denies apps access to human presence sensors used for features such as wake on approach and lock on leave. |
| `P098` | **Disable location override** | `Machine` | Normal |  |
| `P012_USER` | **Disable app access to camera (User)** | `User` | Normal | By default, apps, e.g. the browser Edge, Facebook or Twitter can access the camera on your machine, if one exists. Activating camera access can be useful when you want, for example, to make video chats or conferences. Deactivating may result in not being able to transmit video images. |
| `P012_MACHINE` | **Disable app access to camera (Machine)** | `Machine` | Normal | By default, apps, e.g. the browser Edge, Facebook or Twitter can access the camera on your machine, if one exists. Activating camera access can be useful when you want, for example, to make video chats or conferences. Deactivating may result in not being able to transmit video images. |
| `P013_USER` | **Disable app access to microphone (User)** | `User` | Normal | If you want to use online chat apps or the voice recorder, this function should remain active. If you don't use voice recording or transfer, then deactivate the microphone access to prevent manipulated apps from activating the microphone and recording a conversation without your permission. |
| `P013_MACHINE` | **Disable app access to microphone (Machine)** | `Machine` | Normal | If you want to use online chat apps or the voice recorder, this function should remain active. If you don't use voice recording or transfer, then deactivate the microphone access to prevent manipulated apps from activating the microphone and recording a conversation without your permission. |
| `P062` | **Disable app access to use voice activation** | `Machine` | Normal | When this feature is disabled, apps can no longer be activated by voice commands. This may limit some apps in their function or stop working at all. |
| `P063` | **Disable app access to use voice activation when device is locked** | `Machine` | Normal | When this feature is disabled, apps can no longer be activated by voice commands when the device is locked. This may limit some apps in their function or stop working at all. |
| `P081` | **Disable the standard app for the headset button** | `Machine` | Normal | A button may exist on a headset that restarts the last app when it is pressed. If you accidentally press this button and start an undesired app, this can be a privacy issue. This setting disables this. You can then connect a defined application to the button in the Windows Settings. |
| `P019_USER` | **Disable app access to notifications (User)** | `User` | Normal | Disabling this function means apps will no longer have access to your messages such as SMS or MMS. As a result, some apps may be limited in their functionality or no longer function at all (e.g. messaging apps). |
| `P019_MACHINE` | **Disable app access to notifications (Machine)** | `Machine` | Normal | Disabling this function means apps will no longer have access to your messages such as SMS or MMS. As a result, some apps may be limited in their functionality or no longer function at all (e.g. messaging apps). |
| `P048_USER` | **Disable app access to movements (User)** | `User` | Normal | When this feature is disabled, apps will no longer have access to your movements, which are recorded by so-called motion trackers. This can limit some apps in their function or stop working at all (e.g. fitness apps). |
| `P048_MACHINE` | **Disable app access to movements (Machine)** | `Machine` | Normal | When this feature is disabled, apps will no longer have access to your movements, which are recorded by so-called motion trackers. This can limit some apps in their function or stop working at all (e.g. fitness apps). |
| `P020_USER` | **Disable app access to contacts (User)** | `User` | Normal | Disabling this function means apps will no longer have access to your contacts. As a result, some apps may be limited in their functionality or no longer function at all (e.g. messaging apps). |
| `P020_MACHINE` | **Disable app access to contacts (Machine)** | `Machine` | Normal | Disabling this function means apps will no longer have access to your contacts. As a result, some apps may be limited in their functionality or no longer function at all (e.g. messaging apps). |
| `P011_USER` | **Disable app access to calendar (User)** | `User` | Normal | By deactivating this function, apps will have no access to calendar entries. Some apps may be limited in their functionality or not work at all (e.g. calendar apps). |
| `P011_MACHINE` | **Disable app access to calendar (Machine)** | `Machine` | Normal | By deactivating this function, apps will have no access to calendar entries. Some apps may be limited in their functionality or not work at all (e.g. calendar apps). |
| `P050_USER` | **Disable app access to phone calls (User)** | `User` | Normal | When this feature is disabled, apps will no longer have access to phones connected to this device and will not be able to make calls. This may limit some apps in their function or stop working at all. |
| `P050_MACHINE` | **Disable app access to phone calls (Machine)** | `Machine` | Normal | When this feature is disabled, apps will no longer have access to phones connected to this device and will not be able to make calls. This may limit some apps in their function or stop working at all. |
| `P018_USER` | **Disable app access to call history (User)** | `User` | Normal | Disabling this function means apps will no longer have access to your calling history. As a result, some apps may be limited in their functionality or no longer function at all (e.g. messaging apps). |
| `P018_MACHINE` | **Disable app access to call history (Machine)** | `Machine` | Normal | Disabling this function means apps will no longer have access to your calling history. As a result, some apps may be limited in their functionality or no longer function at all (e.g. messaging apps). |
| `P021_USER` | **Disable app access to email (User)** | `User` | Normal | Disabling this function means apps will no longer have access to your mails. As a result, some apps may be limited in their functionality or no longer function at all (e.g. mail apps). |
| `P021_MACHINE` | **Disable app access to email (Machine)** | `Machine` | Normal | Disabling this function means apps will no longer have access to your mails. As a result, some apps may be limited in their functionality or no longer function at all (e.g. mail apps). |
| `112_USER` | **Disable app access to tasks (User)** | `User` | Normal | Disabling this function means apps will no longer have access to your task lists. As a result, some apps may be limited in their functionality or no longer function at all (e.g. messaging or calendar apps). |
| `112_MACHINE` | **Disable app access to tasks (Machine)** | `Machine` | Normal | Disabling this function means apps will no longer have access to your task lists. As a result, some apps may be limited in their functionality or no longer function at all (e.g. messaging or calendar apps). |
| `P014_USER` | **Disable app access to messages (User)** | `User` | Normal | Apps are denied access to your notification / mails if these functions are deactivated (Emails, SMS, Messenger). This may lead to Messenger apps or email apps not working correctly anymore. |
| `P014_MACHINE` | **Disable app access to messages (Machine)** | `Machine` | Normal | Apps are denied access to your notification / mails if these functions are deactivated (Emails, SMS, Messenger). This may lead to Messenger apps or email apps not working correctly anymore. |
| `P052_USER` | **Disable app access to wireless connections (User)** | `User` | Normal | When this feature is disabled, apps will no longer have access to wireless connections and will not be able to enable or disable them on their own. This may limit some apps in their function or stop working at all. |
| `P052_MACHINE` | **Disable app access to wireless connections (Machine)** | `Machine` | Normal | When this feature is disabled, apps will no longer have access to wireless connections and will not be able to enable or disable them on their own. This may limit some apps in their function or stop working at all. |
| `P054` | **Disable app access to loosely coupled devices** | `User` | Normal | If this feature is disabled, apps must not establish wireless connections that were not previously authorized (e.g. beacons). This may limit some apps in their function or stop working at all. |
| `P029_USER` | **Disable app access to documents (User)** | `User` | Normal | When this feature is disabled, apps will no longer have access to your documents. Some apps may be restricted in your function or may no longer work at all. |
| `P029_MACHINE` | **Disable app access to documents (Machine)** | `Machine` | Normal | When this feature is disabled, apps will no longer have access to your documents. Some apps may be restricted in your function or may no longer work at all. |
| `P030_USER` | **Disable app access to images (User)** | `User` | Normal | When this feature is disabled, apps will no longer have access to your pictures and photos. Some apps may be restricted in your function or may no longer work at all. |
| `P030_MACHINE` | **Disable app access to images (Machine)** | `Machine` | Normal | When this feature is disabled, apps will no longer have access to your pictures and photos. Some apps may be restricted in your function or may no longer work at all. |
| `P031_USER` | **Disable app access to videos (User)** | `User` | Normal | When this feature is disabled, apps will no longer have access to your videos. Some apps may be restricted in your function or may no longer work at all. |
| `P031_MACHINE` | **Disable app access to videos (Machine)** | `Machine` | Normal | When this feature is disabled, apps will no longer have access to your videos. Some apps may be restricted in your function or may no longer work at all. |
| `P032_USER` | **Disable app access to the file system (User)** | `User` | Normal | When this feature is disabled, apps will no longer have access to your file system and therefore your files. Some apps may be restricted in your function or may no longer work at all. |
| `P032_MACHINE` | **Disable app access to the file system (Machine)** | `Machine` | Normal | When this feature is disabled, apps will no longer have access to your file system and therefore your files. Some apps may be restricted in your function or may no longer work at all. |
| `P058_USER` | **Disable app access to wireless technology (User)** | `User` | Normal | When this feature is disabled, apps are no longer allowed to use the PC's wireless technology. This can limit some apps in their function or stop working if they need a data connection. |
| `P058_MACHINE` | **Disable app access to wireless technology (Machine)** | `Machine` | Normal | When this feature is disabled, apps are no longer allowed to use the PC's wireless technology. This can limit some apps in their function or stop working if they need a data connection. |
| `P060_USER` | **Disable app access to eye tracking (User)** | `User` | Normal | When this feature is disabled, apps will no longer be able to track the user's eyes and gaze in front of the device. This may limit some apps in their function or stop working at all. This may affect applications for users with neuromuscular diseases such as ALS, who can control the PC using this functionality. |
| `P060_MACHINE` | **Disable app access to eye tracking (Machine)** | `Machine` | Normal | When this feature is disabled, apps will no longer be able to track the user's eyes and gaze in front of the device. This may limit some apps in their function or stop working at all. This may affect applications for users with neuromuscular diseases such as ALS, who can control the PC using this functionality. |
| `P071_USER` | **Disable the ability for apps to take screenshots (User)** | `User` | Normal | Apps can take screenshots of your Windows desktop or other applications. This may mean that there is private content on these screenshots, which is then further processed by the app. With this setting you can prevent this. |
| `P071_MACHINE` | **Disable the ability for apps to take screenshots (Machine)** | `Machine` | Normal | Apps can take screenshots of your Windows desktop or other applications. This may mean that there is private content on these screenshots, which is then further processed by the app. With this setting you can prevent this. |
| `P073` | **Disable the ability for desktop apps to take screenshots** | `Machine` | Normal | Windows applications (= applications that you have not installed from the Microsoft Store) can take screenshots (screenshots) of your Windows desktop or other applications. This may mean that there is private content on these screenshots, which is then further processed by the Windows application. With this setting you can prevent this. |
| `P074_USER` | **Disable the ability for apps to take screenshots without borders (User)** | `User` | Normal | Apps can take screenshots of your Windows desktop or other applications. This may mean that there is private content on these screenshots, which is then further processed by the app. With this setting, you can prevent an app from disabling the edges of screenshots and thus taking more than desired. |
| `P074_MACHINE` | **Disable the ability for apps to take screenshots without borders (Machine)** | `Machine` | Normal | Apps can take screenshots of your Windows desktop or other applications. This may mean that there is private content on these screenshots, which is then further processed by the app. With this setting, you can prevent an app from disabling the edges of screenshots and thus taking more than desired. |
| `P076` | **Disable the ability for desktop apps to take screenshots without margins** | `Machine` | Normal | Windows applications (= applications that you have not installed from the Microsoft Store) can take screenshots (screenshots) of your Windows desktop or other applications. This may mean that there is private content on these screenshots, which is then further processed by the Windows application. With this setting, you can prevent an app from disabling the edges of screenshots and thus taking more than desired. |
| `P077_USER` | **Disable app access to music libraries (User)** | `User` | Normal | If this feature is turned off, apps will no longer be allowed to access music libraries. Some apps may be restricted in their function or stop working at all if they need access to music files. |
| `P077_MACHINE` | **Disable app access to music libraries (Machine)** | `Machine` | Normal | If this feature is turned off, apps will no longer be allowed to access music libraries. Some apps may be restricted in their function or stop working at all if they need access to music files. |
| `P079_USER` | **Disable app access to downloads folder (User)** | `User` | Normal | If this feature is disabled, apps will no longer be allowed to access the downloads folder. Some apps may be restricted in their function or may stop working at all if they need access to this directory. |
| `P079_MACHINE` | **Disable app access to downloads folder (Machine)** | `Machine` | Normal | If this feature is disabled, apps will no longer be allowed to access the downloads folder. Some apps may be restricted in their function or may stop working at all if they need access to this directory. |
| `P024` | **Prohibit apps from running in the background** | `Machine` | Advanced | If this feature is disabled, apps will no longer be able to run in the background. This means that they are always terminated immediately and can no longer process or send messages in the background. As a result, notifications are no longer displayed on the desktop and in the Action Center. Only the number of messages is still shown. If you want to prevent this, deactivate this function. This also saves electricity, which can be relevant for mobile devices. |
| `P086_USER` | **Disable app access to passkeys (User)** | `User` | Advanced | If this feature is disabled, apps and websites will no longer be allowed to access the passkeys stored on this device. Signing in with a passkey will then no longer be possible and you will have to use your password or another sign-in method instead. |
| `P086_MACHINE` | **Disable app access to passkeys (Machine)** | `Machine` | Advanced | If this feature is disabled, apps and websites will no longer be allowed to access the passkeys stored on this device. Signing in with a passkey will then no longer be possible and you will have to use your password or another sign-in method instead. |
| `P089_USER` | **Disable app access to the list of stored passkeys (User)** | `User` | Advanced | If this feature is disabled, apps and websites will no longer be allowed to determine which passkeys are stored on this device. Websites may then no longer offer you a passkey sign-in even though a passkey exists. |
| `P089_MACHINE` | **Disable app access to the list of stored passkeys (Machine)** | `Machine` | Advanced | If this feature is disabled, apps and websites will no longer be allowed to determine which passkeys are stored on this device. Websites may then no longer offer you a passkey sign-in even though a passkey exists. |
| `P087_USER` | **Disable app access to Bluetooth devices (User)** | `User` | Advanced | If this feature is disabled, apps will no longer be allowed to communicate with Bluetooth devices. Apps that rely on Bluetooth accessories such as headphones, controllers or fitness trackers may stop working. |
| `P087_MACHINE` | **Disable app access to Bluetooth devices (Machine)** | `Machine` | Advanced | If this feature is disabled, apps will no longer be allowed to communicate with Bluetooth devices. Apps that rely on Bluetooth accessories such as headphones, controllers or fitness trackers may stop working. |
| `P088_USER` | **Disable app access to human interface devices (User)** | `User` | Advanced | If this feature is disabled, apps will no longer be allowed to access human interface devices (HID). Apps that use game controllers or other special input hardware may stop working. Your keyboard and mouse are not affected: Windows reserves these devices for the system, so they remain available in any case. |
| `P088_MACHINE` | **Disable app access to human interface devices (Machine)** | `Machine` | Advanced | If this feature is disabled, apps will no longer be allowed to access human interface devices (HID). Apps that use game controllers or other special input hardware may stop working. Your keyboard and mouse are not affected: Windows reserves these devices for the system, so they remain available in any case. |
| `P090_USER` | **Disable app access to custom sensors (User)** | `User` | Advanced | If this feature is disabled, apps will no longer be allowed to read custom sensors built into your device. Apps that evaluate sensor data may be restricted in their function. |
| `P090_MACHINE` | **Disable app access to custom sensors (Machine)** | `Machine` | Advanced | If this feature is disabled, apps will no longer be allowed to read custom sensors built into your device. Apps that evaluate sensor data may be restricted in their function. |
| `P091_USER` | **Disable app access to serial ports (User)** | `User` | Advanced | If this feature is disabled, apps will no longer be allowed to use serial ports. Apps that communicate with measuring instruments, microcontrollers or other serial hardware may stop working. |
| `P091_MACHINE` | **Disable app access to serial ports (Machine)** | `Machine` | Advanced | If this feature is disabled, apps will no longer be allowed to use serial ports. Apps that communicate with measuring instruments, microcontrollers or other serial hardware may stop working. |
| `P092_USER` | **Disable app access to USB devices (User)** | `User` | Advanced | If this feature is disabled, apps will no longer be allowed to communicate directly with USB devices. Apps that use USB hardware such as printers, scanners or programming adapters may stop working. |
| `P092_MACHINE` | **Disable app access to USB devices (Machine)** | `Machine` | Advanced | If this feature is disabled, apps will no longer be allowed to communicate directly with USB devices. Apps that use USB hardware such as printers, scanners or programming adapters may stop working. |
| `P093_USER` | **Disable app access to Wi-Fi information (User)** | `User` | Advanced | If this feature is disabled, apps will no longer be allowed to read information about nearby Wi-Fi networks. This data can be used to determine your location, but some apps may be restricted in their function. |
| `P093_MACHINE` | **Disable app access to Wi-Fi information (Machine)** | `Machine` | Advanced | If this feature is disabled, apps will no longer be allowed to read information about nearby Wi-Fi networks. This data can be used to determine your location, but some apps may be restricted in their function. |
| `P094_USER` | **Disable app access to Wi-Fi Direct (User)** | `User` | Advanced | If this feature is disabled, apps will no longer be allowed to establish direct Wi-Fi connections to other devices. Features such as wireless displays or direct file transfer may stop working. |
| `P094_MACHINE` | **Disable app access to Wi-Fi Direct (Machine)** | `Machine` | Advanced | If this feature is disabled, apps will no longer be allowed to establish direct Wi-Fi connections to other devices. Features such as wireless displays or direct file transfer may stop working. |

[↑ Back to Top](#-categories-overview)

---

## Cortana & Search

*Category contains **17** manageable settings.*

| ID | Setting / Title | Scope | Impact | Description |
| :--- | :--- | :---: | :---: | :--- |
| `C001_USER` | **Disable and reset Cortana (User)** | `User` | 🟢 Recommended (Safe) | If you don't want the personal assistant Cortana, then deactivate this function. This prevents Microsoft receiving information like contacts, current calendar events, language patterns, handwriting samples and your entry history. |
| `C001_MACHINE` | **Disable and reset Cortana (Machine)** | `Machine` | 🟢 Recommended (Safe) | If you don't want the personal assistant Cortana, then deactivate this function. This prevents Microsoft receiving information like contacts, current calendar events, language patterns, handwriting samples and your entry history. |
| `C002` | **Disable Input Personalization** | `Machine` | 🟢 Recommended (Safe) |  |
| `C013` | **Disable online speech recognition** | `Machine` | 🟢 Recommended (Safe) |  |
| `C007` | **Cortana and search are disallowed to use location** | `Machine` | 🟢 Recommended (Safe) | Cortana uses your geographical location to present your search results accordingly. You have the option of disabling this feature if you don’t wish to indicate your location. |
| `SRCH_BING_START` | **Disable web search from Windows Desktop Search** | `Machine` | 🟢 Recommended (Safe) | When starting a Windows desktop search, results from the web will also be presented. This setting will allow you to limit the results of your search to your computer only. |
| `C009` | **Disable display web results in Search** | `Machine` | 🟢 Recommended (Safe) | Cortana can search throughout the web for you. You can easily disable this feature by using this setting. |
| `C010` | **Disable download and updates of speech recognition and speech synthesis models** | `Machine` | 🟢 Recommended (Safe) | If you don't wish to use Cortana, this option will also allow you to disable the Cortana module from refreshing and providing downloads. |
| `SRCH_CLOUD` | **Disable cloud search** | `Machine` | 🟢 Recommended (Safe) | Using Cortana for searches will also involve affiliated cloud sources such as OneDrive or SharePoint. This will result in your local searches being transferred and carried out on Microsoft Servers. Disabling searches in the cloud will prevent this from happening. |
| `SRCH_CORTANA` | **Disable Cortana above lock screen** | `Machine` | 🟢 Recommended (Safe) |  |
| `AI_SEARCH_HIGHLIGHTS_WSB` | **Disable the search highlights in the taskbar** | `Machine` | 🟢 Recommended (Safe) | So-called search highlights are displayed in the taskbar, which refer to current events and search trends. This can not only make the taskbar more confusing visually, but also transmit information to Microsoft. This can be disabled with this setting. |
| `M025` | **Disable search with AI in search box** | `Machine` | 🟢 Recommended (Safe) | In the search field of the taskbar, a Bing icon is displayed that enables searching by means of AI (artificial intelligence). For this, a separate browser window is opened and you can enter a natural language question there. Personal information is transmitted in the process. This AI option can be disabled with this setting. |
| `M003_USER` | **Disable extension of Windows search with Bing (User)** | `User` | 🟢 Recommended (Safe) | If you want to look for a local App or a setting in the Windows search, but don't type in the exact name, Windows will look for an answer by default using Bing, instead of using your local hard disk for adequate results. This will deactivate that function.  <u>Note:</u> Windows Search and Explorer may sporadically reset this setting during search indexer restarts, feature updates, or Group Policy refresh cycles. |
| `M003_MACHINE` | **Disable extension of Windows search with Bing (Machine)** | `Machine` | 🟢 Recommended (Safe) | If you want to look for a local App or a setting in the Windows search, but don't type in the exact name, Windows will look for an answer by default using Bing, instead of using your local hard disk for adequate results. This will deactivate that function.  <u>Note:</u> Windows Search and Explorer may sporadically reset this setting during search indexer restarts, feature updates, or Group Policy refresh cycles. |
| `M029` | **Disable Microsoft account cloud content search** | `Machine` | 🟢 Recommended (Safe) | This setting disables Microsoft account cloud content results in Windows Search for the current user. |
| `M030` | **Disable work or school cloud content search** | `Machine` | 🟢 Recommended (Safe) | This setting disables work or school account cloud content results in Windows Search for the current user. |
| `M031` | **Disable device search history** | `Machine` | 🟢 Recommended (Safe) | This setting disables search history stored on the device for the current user. |

[↑ Back to Top](#-categories-overview)

---

## Gaming & Xbox

*Category contains **1** manageable settings.*

| ID | Setting / Title | Scope | Impact | Description |
| :--- | :--- | :---: | :---: | :--- |
| `G001` | **Disable Xbox Game Bar and Game DVR** | `Machine` | Normal |  |

[↑ Back to Top](#-categories-overview)

---

## Location & Sensors

*Category contains **7** manageable settings.*

| ID | Setting / Title | Scope | Impact | Description |
| :--- | :--- | :---: | :---: | :--- |
| `L001` | **Disable functionality to locate the system** | `Machine` | 🟢 Recommended (Safe) | Locations services is used so that apps or websites can show you results based on your location, e.g. directions or restaurants in your neighborhood.   <u>Note:</u> Enabling this setting may disable the Windows 11 Night Light feature, which relies on the location service to determine sunrise and sunset times for automatic scheduling. |
| `L003` | **Disable scripting functionality to locate the system** | `Machine` | 🟢 Recommended (Safe) | Locations services is used so that apps or websites can show you results based on your location, e.g. directions or restaurants in your neighborhood. |
| `L004` | **Disable sensors for locating the system and its orientation** | `Machine` | Normal | Assuming they are available, GPS receivers and gyroscope sensors will be deactivated. For Tablet PCs, this could mean that screen rotation will no longer be recognized. It should not be activated if this function is required. |
| `L005_USER` | **Disable Windows Geolocation Service (User)** | `User` | Normal | The geolocation service in Windows manages the current location of the system and defines geographical boundaries (so-called “geofencing“). Deactivating it means applications can no longer access the geographical location through this service. |
| `L005_MACHINE` | **Disable Windows Geolocation Service (Machine)** | `Machine` | Normal | The geolocation service in Windows manages the current location of the system and defines geographical boundaries (so-called “geofencing“). Deactivating it means applications can no longer access the geographical location through this service. |
| `L008` | **Disable Find My Device** | `Machine` | 🟢 Recommended (Safe) | This setting disables Find My Device so Windows does not periodically send the device location to the associated Microsoft account. |
| `L007` | **Disable app access to your location** | `Machine` | Normal | This function allows apps to access your location. Some apps require this in order to deliver their content in your language or deliver content based on your geographical location. Deactivating this function can mean that some apps display content in the wrong language or deliver the wrong geographical content, and in the worst case may render some apps unusable. |

[↑ Back to Top](#-categories-overview)

---

## Lock Screen & Desktop

*Category contains **3** manageable settings.*

| ID | Setting / Title | Scope | Impact | Description |
| :--- | :--- | :---: | :---: | :--- |
| `LOCK_SPOTLIGHT_ADS` | **Disable Windows Spotlight** | `Machine` | 🟢 Recommended (Safe) | Windows Spotlight provides (daily) changing pictures on your lock screen. These are taken from Microsoft Bing. You can rate these pictures. When doing so, information will be sent to Microsoft that can clearly identify you personally as well as your computer. Turning off this function is recommended. Note: Windows Spotlight also stops working if “Disable fun facts, tips, tricks, and more on your lock screen” is enabled. |
| `K002` | **Disable fun facts, tips, tricks, and more on your lock screen** | `Machine` | 🟢 Recommended (Safe) | Along with tips and tricks for using Windows, the lock screen also fades in advertisements and additional information. These will send a lot of information onto Microsoft that can be used to identify your computer as well as you personally. That’s why this setting should be disabled. Note: Windows Spotlight on the lock screen depends on this content. If this setting is enabled, Windows Spotlight stops working and Windows switches the lock screen to a picture. Leave this setting disabled if you want to keep using Windows Spotlight. |
| `K005` | **Disable notifications on lock screen** | `Machine` | 🟢 Recommended (Safe) | Notifications from apps can be displayed on the lock screen. These might contain private information that others could read without having to be logged onto the computer. Setting this setting disables showing notifications. |

[↑ Back to Top](#-categories-overview)

---

## Microsoft Edge

*Category contains **102** manageable settings.*

| ID | Setting / Title | Scope | Impact | Description |
| :--- | :--- | :---: | :---: | :--- |
| `E001_USER` | **Disable tracking in the web (User)** | `User` | 🟢 Recommended (Safe) |  |
| `E001_MACHINE` | **Disable tracking in the web (Machine)** | `Machine` | 🟢 Recommended (Safe) |  |
| `E002` | **Disable page prediction** | `Machine` | 🟢 Recommended (Safe) | When using page prediction, pages linked to the page you're visiting will be automatically loaded in the background. This is supposed to make surfing faster but it also transmits information onto sites that you may never want to visit. That's why we recommend disabling this function. |
| `E003_USER` | **Disable search and website suggestions (User)** | `User` | 🟢 Recommended (Safe) | While entering searches or URIs in Edge, matching suggestions will appear automatically. This is done by transmitting the input to Microsoft where it must be evaluated in order to create these suggestions. Such a transfer of data enables conclusions to be made regarding surfing behavior. |
| `E003_MACHINE` | **Disable search and website suggestions (Machine)** | `Machine` | 🟢 Recommended (Safe) | While entering searches or URIs in Edge, matching suggestions will appear automatically. This is done by transmitting the input to Microsoft where it must be evaluated in order to create these suggestions. Such a transfer of data enables conclusions to be made regarding surfing behavior. |
| `E008` | **Disable Cortana in Microsoft Edge** | `Machine` | 🟢 Recommended (Safe) |  |
| `E007_USER` | **Disable automatic completion of web addresses in address bar (User)** | `User` | 🟢 Recommended (Safe) | Automatic completion in the address lines of Edge can possibly transmit data concerning user behavior. As a result, information regarding your surfing activity might also be available to others who use this computer under your user account. By disabling this feature, no suggestions will be made for completing any addresses while entering a web address. |
| `E007_MACHINE` | **Disable automatic completion of web addresses in address bar (Machine)** | `Machine` | 🟢 Recommended (Safe) | Automatic completion in the address lines of Edge can possibly transmit data concerning user behavior. As a result, information regarding your surfing activity might also be available to others who use this computer under your user account. By disabling this feature, no suggestions will be made for completing any addresses while entering a web address. |
| `E010` | **Disable showing search history** | `Machine` | 🟢 Recommended (Safe) | Use these settings to disable showing search history. This feature is usually helpful but such information can also be shown to other users, especially when sharing a PC. |
| `E011_USER` | **Disable user feedback in toolbar (User)** | `User` | 🟢 Recommended (Safe) | Edge (Chromium) displays a smiley on the toolbar for sending feedback to Microsoft. This setting allows you to hide the smiley. After changing the setting, the browser must be restarted for it to take effect. |
| `E011_MACHINE` | **Disable user feedback in toolbar (Machine)** | `Machine` | 🟢 Recommended (Safe) | Edge (Chromium) displays a smiley on the toolbar for sending feedback to Microsoft. This setting allows you to hide the smiley. After changing the setting, the browser must be restarted for it to take effect. |
| `E012_USER` | **Disable storing and autocompleting of credit card data on websites (User)** | `User` | 🟢 Recommended (Safe) | Microsoft Edge can automatically store credit card information and fill it out on later purchases. To do this, the data must be stored reversibly on the local machine, so this poses a potential security risk. |
| `E012_MACHINE` | **Disable storing and autocompleting of credit card data on websites (Machine)** | `Machine` | 🟢 Recommended (Safe) | Microsoft Edge can automatically store credit card information and fill it out on later purchases. To do this, the data must be stored reversibly on the local machine, so this poses a potential security risk. |
| `E009_USER` | **Disable form suggestions (User)** | `User` | Normal | Microsoft Edge can suggest previous entries for easier completion when filling out forms. This is usually helpful but such information can also be displayed to other users, especially when sharing a PC. |
| `E009_MACHINE` | **Disable form suggestions (Machine)** | `Machine` | Normal | Microsoft Edge can suggest previous entries for easier completion when filling out forms. This is usually helpful but such information can also be displayed to other users, especially when sharing a PC. |
| `E004` | **Disable sites saving protected media licenses on my device** | `Machine` | Normal | It is sometimes necessary to save information for protected music or video content (so-called Digital Rights Management = DRM) in order to play them on a device. This also requires a specific ID to identify the machine. Enable this setting if you want to stop this from happening. |
| `E005` | **Do not optimize web search results on the task bar for screen reader** | `Machine` | Normal | When using the task bar to do a web search, it's possible to make settings that define whether the results should be displayed (non-optimized) in Edge or (optimized) in Internet Explorer. The latter is compatible with the screen reader for Windows and allows visually-impaired users to read websites. If you're not intending to use this feature, we recommend disabling it to avoid having to use Internet Explorer as well. Depending on the needs of visually-impaired users, this setting is only somewhat recommended. |
| `E013` | **Disable Microsoft Edge launch in the background** | `Machine` | Normal | Microsoft Edge can start in the background to improve performance when the system is idle. This happens when Windows starts and whenever Edge is closed. Disabling this may reduce Edge performance while making the system itself faster. |
| `E014` | **Disable loading the start and new tab pages in the background** | `Machine` | Normal | Microsoft Edge can preload the Start and New Tab pages in the background to improve performance when the system is idle. This happens when Windows starts and whenever Edge is closed. Disabling this may reduce Edge performance while making the system itself faster. |
| `E006_USER` | **Disable SmartScreen Filter (User)** | `User` | Advanced | The SmartScreen Filter protects you from accessing malicious websites and downloads whenever you're surfing with Edge. In order to do this, information (e.g., the URL) will be sent to Microsoft that allows it to identify such dangerous content. Disabling this function provides more privacy but it also gives you less protection while surfing. That's why we recommend your leaving this function enabled. |
| `E006_MACHINE` | **Disable SmartScreen Filter (Machine)** | `Machine` | Advanced | The SmartScreen Filter protects you from accessing malicious websites and downloads whenever you're surfing with Edge. In order to do this, information (e.g., the URL) will be sent to Microsoft that allows it to identify such dangerous content. Disabling this function provides more privacy but it also gives you less protection while surfing. That's why we recommend your leaving this function enabled. |
| `E115_USER` | **Disable check for saved payment methods by sites (User)** | `User` | 🟢 Recommended (Safe) | Websites can check whether the current user has stored payment methods in the browser. By setting this policy, you can prevent verification so that no information about it is transmitted from the browser to the Web site. |
| `E115_MACHINE` | **Disable check for saved payment methods by sites (Machine)** | `Machine` | 🟢 Recommended (Safe) | Websites can check whether the current user has stored payment methods in the browser. By setting this policy, you can prevent verification so that no information about it is transmitted from the browser to the Web site. |
| `E116_USER` | **Disable sending info about websites visited (User)** | `User` | 🟢 Recommended (Safe) | Microsoft Edge sends information about the websites you visit to Microsoft to improve search and products. By setting this policy, you can prevent sending so that no information is transmitted by the browser. |
| `E116_MACHINE` | **Disable sending info about websites visited (Machine)** | `Machine` | 🟢 Recommended (Safe) | Microsoft Edge sends information about the websites you visit to Microsoft to improve search and products. By setting this policy, you can prevent sending so that no information is transmitted by the browser. |
| `EDGE_METRICS_USER` | **Disable sending data about browser usage (User)** | `User` | 🟢 Recommended (Safe) | Microsoft Edge sends information about usage behavior and crashes to Microsoft to improve the product. By setting this policy, you can prevent sending so that no information is transmitted by the browser. |
| `EDGE_METRICS_MACHINE` | **Disable sending data about browser usage (Machine)** | `Machine` | 🟢 Recommended (Safe) | Microsoft Edge sends information about usage behavior and crashes to Microsoft to improve the product. By setting this policy, you can prevent sending so that no information is transmitted by the browser. |
| `E118_USER` | **Disable personalizing advertising, search, news and other services (User)** | `User` | 🟢 Recommended (Safe) | Microsoft Edge sends information about usage behavior to Microsoft to improve advertising, search, news, and other Microsoft services. Enable this policy to prevent the browser from sending this information. |
| `E118_MACHINE` | **Disable personalizing advertising, search, news and other services (Machine)** | `Machine` | 🟢 Recommended (Safe) | Microsoft Edge sends information about usage behavior to Microsoft to improve advertising, search, news, and other Microsoft services. Enable this policy to prevent the browser from sending this information. |
| `E121_USER` | **Disable suggestions from local providers (User)** | `User` | 🟢 Recommended (Safe) | Microsoft Edge can display suggestions from so-called suggestion providers in the address bar, favorites, and browsing history. By setting this policy, you can prevent this display. |
| `E121_MACHINE` | **Disable suggestions from local providers (Machine)** | `Machine` | 🟢 Recommended (Safe) | Microsoft Edge can display suggestions from so-called suggestion providers in the address bar, favorites, and browsing history. By setting this policy, you can prevent this display. |
| `E123_USER` | **Disable shopping assistant in Microsoft Edge (User)** | `User` | 🟢 Recommended (Safe) | When visiting websites, Microsoft Edge can automatically search for coupons or discounts, or compare prices. For this purpose, data must be transmitted in the background to appropriate servers in order to provide this function. This can lead to the transmission of private information, which is not desired. |
| `E123_MACHINE` | **Disable shopping assistant in Microsoft Edge (Machine)** | `Machine` | 🟢 Recommended (Safe) | When visiting websites, Microsoft Edge can automatically search for coupons or discounts, or compare prices. For this purpose, data must be transmitted in the background to appropriate servers in order to provide this function. This can lead to the transmission of private information, which is not desired. |
| `E124_USER` | **Disable Edge bar (User)** | `User` | 🟢 Recommended (Safe) | The input bar from Microsoft Edge allows web pages to be accessed directly from the search box that appears on the desktop. If you want to disable this bar, you can do so with this option. |
| `E124_MACHINE` | **Disable Edge bar (Machine)** | `Machine` | 🟢 Recommended (Safe) | The input bar from Microsoft Edge allows web pages to be accessed directly from the search box that appears on the desktop. If you want to disable this bar, you can do so with this option. |
| `E128_USER` | **Disable Sidebar in Microsoft Edge (User)** | `User` | 🟢 Recommended (Safe) | The sidebar on the right side of Microsoft Edge is enabled by default and is used for quick access to applications, but also to search with Bing. That is why it is represented by the Bing icon when it is closed. To remove this edge bar, enable this setting. The next time you launch Microsoft Edge, the setting will be applied. |
| `E128_MACHINE` | **Disable Sidebar in Microsoft Edge (Machine)** | `Machine` | 🟢 Recommended (Safe) | The sidebar on the right side of Microsoft Edge is enabled by default and is used for quick access to applications, but also to search with Bing. That is why it is represented by the Bing icon when it is closed. To remove this edge bar, enable this setting. The next time you launch Microsoft Edge, the setting will be applied. |
| `E130_USER` | **Disable Enhanced Spell Checking (User)** | `User` | 🟢 Recommended (Safe) | This setting disables the enhanced spelling and grammar check provided by Microsoft Editor. Instead, the basic, local spell check is used, which does not rely on the cloud. |
| `E130_MACHINE` | **Disable Enhanced Spell Checking (Machine)** | `Machine` | 🟢 Recommended (Safe) | This setting disables the enhanced spelling and grammar check provided by Microsoft Editor. Instead, the basic, local spell check is used, which does not rely on the cloud. |
| `E132_USER` | **Hide first run experience and splash screen (User)** | `User` | 🟢 Recommended (Safe) | This setting hides the first-run experience and splash screen when Microsoft Edge is launched for the first time, preventing promotional content from being displayed. |
| `E132_MACHINE` | **Hide first run experience and splash screen (Machine)** | `Machine` | 🟢 Recommended (Safe) | This setting hides the first-run experience and splash screen when Microsoft Edge is launched for the first time, preventing promotional content from being displayed. |
| `112_NewEdge_SpotlightExperiencesAndRecommendationsEnabled` | **Disable spotlight experiences and recommendations** | `User` | 🟢 Recommended (Safe) | This setting disables promotional spotlight experiences and recommendations from Microsoft, reducing visual clutter and potential privacy concerns. |
| `E134_USER` | **Disable automatic sign-in from web to browser (User)** | `User` | 🟢 Recommended (Safe) | This setting prevents automatic sign-in to the browser when signing into Microsoft websites, improving privacy by separating web and browser accounts. |
| `E134_MACHINE` | **Disable automatic sign-in from web to browser (Machine)** | `Machine` | 🟢 Recommended (Safe) | This setting prevents automatic sign-in to the browser when signing into Microsoft websites, improving privacy by separating web and browser accounts. |
| `E135_USER` | **Disable Bing Chat on new tab page (User)** | `User` | 🟢 Recommended (Safe) | This setting disables Bing Chat on the new tab page, reducing AI features and potential data sharing. |
| `E135_MACHINE` | **Disable Bing Chat on new tab page (Machine)** | `Machine` | 🟢 Recommended (Safe) | This setting disables Bing Chat on the new tab page, reducing AI features and potential data sharing. |
| `E136_USER` | **Disable content on new tab page (User)** | `User` | 🟢 Recommended (Safe) | This setting disables content on the new tab page such as news feed and promotional information, showing a clean, minimal new tab page. |
| `E136_MACHINE` | **Disable content on new tab page (Machine)** | `Machine` | 🟢 Recommended (Safe) | This setting disables content on the new tab page such as news feed and promotional information, showing a clean, minimal new tab page. |
| `112_NewEdge_AIGenThemesEnabled` | **Disable AI-generated themes** | `User` | 🟢 Recommended (Safe) | This setting disables the AI-generated themes feature in Edge, reducing unnecessary AI processing and potential data sharing. |
| `E138_USER` | **Disable built-in AI APIs for websites (User)** | `User` | 🟢 Recommended (Safe) | This setting disables built-in AI APIs that websites can access, preventing websites from using Edge's AI features. |
| `E138_MACHINE` | **Disable built-in AI APIs for websites (Machine)** | `Machine` | 🟢 Recommended (Safe) | This setting disables built-in AI APIs that websites can access, preventing websites from using Edge's AI features. |
| `E139_USER` | **Disable inline Compose feature (User)** | `User` | 🟢 Recommended (Safe) | This setting disables the inline Compose feature (AI writing assistant), reducing AI-based data processing. |
| `E139_MACHINE` | **Disable inline Compose feature (Machine)** | `Machine` | 🟢 Recommended (Safe) | This setting disables the inline Compose feature (AI writing assistant), reducing AI-based data processing. |
| `E140_USER` | **Disable Copilot access to page context (User)** | `User` | 🟢 Recommended (Safe) | This setting disables Copilot access to page context, preventing sending page content to Copilot AI. |
| `E140_MACHINE` | **Disable Copilot access to page context (Machine)** | `Machine` | 🟢 Recommended (Safe) | This setting disables Copilot access to page context, preventing sending page content to Copilot AI. |
| `E141_USER` | **Disable prompts to make Edge the default browser (User)** | `User` | 🟢 Recommended (Safe) | This setting disables prompts to make Edge the default browser, reducing unwanted notifications. |
| `E141_MACHINE` | **Disable prompts to make Edge the default browser (Machine)** | `Machine` | 🟢 Recommended (Safe) | This setting disables prompts to make Edge the default browser, reducing unwanted notifications. |
| `E142_USER` | **Disable default browser campaigns (User)** | `User` | 🟢 Recommended (Safe) | This setting disables campaigns to set Edge as the default browser, preventing promotional interruptions. |
| `E142_MACHINE` | **Disable default browser campaigns (Machine)** | `Machine` | 🟢 Recommended (Safe) | This setting disables campaigns to set Edge as the default browser, preventing promotional interruptions. |
| `E143_USER` | **Disable diagnostic data collection (User)** | `User` | 🟢 Recommended (Safe) | This setting disables diagnostic data collection, minimizing data sent to Microsoft. Options are: 0=Off, 1=Required, 2=Optional. |
| `E143_MACHINE` | **Disable diagnostic data collection (Machine)** | `Machine` | 🟢 Recommended (Safe) | This setting disables diagnostic data collection, minimizing data sent to Microsoft. Options are: 0=Off, 1=Required, 2=Optional. |
| `EDGE_SHOPPING_USER` | **Disable shopping assistant (User)** | `User` | 🟢 Recommended (Safe) | This setting disables the shopping assistant feature, preventing automatic price comparison and coupon suggestions. |
| `EDGE_SHOPPING_MACHINE` | **Disable shopping assistant (Machine)** | `Machine` | 🟢 Recommended (Safe) | This setting disables the shopping assistant feature, preventing automatic price comparison and coupon suggestions. |
| `E145_USER` | **Hide Microsoft 365 Copilot chat icon (User)** | `User` | 🟢 Recommended (Safe) | This setting hides the Microsoft 365 Copilot chat icon from the browser interface. |
| `E145_MACHINE` | **Hide Microsoft 365 Copilot chat icon (Machine)** | `Machine` | 🟢 Recommended (Safe) | This setting hides the Microsoft 365 Copilot chat icon from the browser interface. |
| `E146_USER` | **Hide Microsoft Rewards (User)** | `User` | 🟢 Recommended (Safe) | This setting hides Microsoft Rewards notifications and features, reducing promotional content. |
| `E146_MACHINE` | **Hide Microsoft Rewards (Machine)** | `Machine` | 🟢 Recommended (Safe) | This setting hides Microsoft Rewards notifications and features, reducing promotional content. |
| `E147_USER` | **Disable recommendations in settings (User)** | `User` | 🟢 Recommended (Safe) | This setting disables recommendations in settings and other areas, reducing promotional content. |
| `E147_MACHINE` | **Disable recommendations in settings (Machine)** | `Machine` | 🟢 Recommended (Safe) | This setting disables recommendations in settings and other areas, reducing promotional content. |
| `E148_USER` | **Disable cloud-based tab services (User)** | `User` | 🟢 Recommended (Safe) | This setting disables cloud-based tab services, preventing syncing tab data to the Microsoft cloud. |
| `E148_MACHINE` | **Disable cloud-based tab services (Machine)** | `Machine` | 🟢 Recommended (Safe) | This setting disables cloud-based tab services, preventing syncing tab data to the Microsoft cloud. |
| `E149_USER` | **Disable text prediction in forms (User)** | `User` | 🟢 Recommended (Safe) | This setting disables text prediction features in forms, reducing AI-based text analysis. |
| `E149_MACHINE` | **Disable text prediction in forms (Machine)** | `Machine` | 🟢 Recommended (Safe) | This setting disables text prediction features in forms, reducing AI-based text analysis. |
| `E150_USER` | **Disable visual search (User)** | `User` | 🟢 Recommended (Safe) | This setting disables the visual search feature, preventing sending images to Bing for search. |
| `E150_MACHINE` | **Disable visual search (Machine)** | `Machine` | 🟢 Recommended (Safe) | This setting disables the visual search feature, preventing sending images to Bing for search. |
| `E151_USER` | **Disable AI-powered history search (User)** | `User` | 🟢 Recommended (Safe) | This setting disables AI-powered search in browsing history, preventing AI processing of browsing history. |
| `E151_MACHINE` | **Disable AI-powered history search (Machine)** | `Machine` | 🟢 Recommended (Safe) | This setting disables AI-powered search in browsing history, preventing AI processing of browsing history. |
| `E156_USER` | **Disable Edge Secure Network (built-in VPN) (User)** | `User` | 🟢 Recommended (Safe) | This setting disables the Microsoft Edge Secure Network (built-in VPN) feature, which routes network traffic through Microsoft's servers. Disabling it prevents any data from being transmitted via this service. |
| `E156_MACHINE` | **Disable Edge Secure Network (built-in VPN) (Machine)** | `Machine` | 🟢 Recommended (Safe) | This setting disables the Microsoft Edge Secure Network (built-in VPN) feature, which routes network traffic through Microsoft's servers. Disabling it prevents any data from being transmitted via this service. |
| `E152_USER` | **Allow user control of local AI features (User)** | `User` | 🟢 Recommended (Safe) | This setting allows users to control local AI foundational model features in Microsoft Edge. |
| `E152_MACHINE` | **Allow user control of local AI features (Machine)** | `Machine` | 🟢 Recommended (Safe) | This setting allows users to control local AI foundational model features in Microsoft Edge. |
| `E119_USER` | **Disable use of web service to resolve navigation errors (User)** | `User` | Normal | If there is an error navigating the Microsoft Edge (e.g. due to an incorrectly entered web address), a connection is established to a web service to correct this error (e.g. suggestion of the correct web address). By setting this policy, you can prevent sending so that no information is transmitted by the browser.  This setting is recommended because entering a Web address incorrectly may result in a fake Web page that has malicious potential. |
| `E119_MACHINE` | **Disable use of web service to resolve navigation errors (Machine)** | `Machine` | Normal | If there is an error navigating the Microsoft Edge (e.g. due to an incorrectly entered web address), a connection is established to a web service to correct this error (e.g. suggestion of the correct web address). By setting this policy, you can prevent sending so that no information is transmitted by the browser.  This setting is recommended because entering a Web address incorrectly may result in a fake Web page that has malicious potential. |
| `E120_USER` | **Disable suggestion of similar sites when website cannot be found (User)** | `User` | Normal | If there is an error navigating the Microsoft Edge (e.g. due to an incorrectly entered web address), then a similar Web page is suggested for the original input. By setting this policy, you can prevent sending so that no information is transmitted by the browser.  This setting is recommended because entering a Web address incorrectly may result in a fake Web page that has malicious potential. |
| `E120_MACHINE` | **Disable suggestion of similar sites when website cannot be found (Machine)** | `Machine` | Normal | If there is an error navigating the Microsoft Edge (e.g. due to an incorrectly entered web address), then a similar Web page is suggested for the original input. By setting this policy, you can prevent sending so that no information is transmitted by the browser.  This setting is recommended because entering a Web address incorrectly may result in a fake Web page that has malicious potential. |
| `E122_USER` | **Disable preload of pages for faster browsing and searching (User)** | `User` | Normal | Microsoft Edge can predict which page to load by entering the web address in the address field. This is to do this, certain services are preconfigured in the background to speed up loading. By setting this policy, you can prevent this preloading.  This setting is not recommended because it slows down the speed of searching and displaying a Web page. |
| `E122_MACHINE` | **Disable preload of pages for faster browsing and searching (Machine)** | `Machine` | Normal | Microsoft Edge can predict which page to load by entering the web address in the address field. This is to do this, certain services are preconfigured in the background to speed up loading. By setting this policy, you can prevent this preloading.  This setting is not recommended because it slows down the speed of searching and displaying a Web page. |
| `E125_USER` | **Disable saving passwords for websites (User)** | `User` | Normal | Microsoft Edge stores passwords in its own password manager. These are automatically filled in when you visit a website for which a password is stored. If this setting is deactivated, no new passwords will be saved in the future, but existing ones will continue to be used. |
| `E125_MACHINE` | **Disable saving passwords for websites (Machine)** | `Machine` | Normal | Microsoft Edge stores passwords in its own password manager. These are automatically filled in when you visit a website for which a password is stored. If this setting is deactivated, no new passwords will be saved in the future, but existing ones will continue to be used. |
| `E126_USER` | **Disable site safety services for more information about a visited website (User)** | `User` | Normal | By clicking on the lock in the address bar of Microsoft Edge, you can get more safety information about a website. For this purpose, information is transmitted to Microsoft Bing, which could disclose information. Because this information can be useful for evaluating a website, this setting is only conditionally recommended. |
| `E126_MACHINE` | **Disable site safety services for more information about a visited website (Machine)** | `Machine` | Normal | By clicking on the lock in the address bar of Microsoft Edge, you can get more safety information about a website. For this purpose, information is transmitted to Microsoft Bing, which could disclose information. Because this information can be useful for evaluating a website, this setting is only conditionally recommended. |
| `E131` | **Disable automatic redirection from Internet Explorer to Microsoft Edge** | `Machine` | Normal | This setting disables the IEToEdge Browser Helper Object (BHO), preventing the automatic redirection from Internet Explorer to Microsoft Edge. Please note that this may cause issues with applications that still rely on Internet Explorer. |
| `E153_USER` | **Disable startup boost (User)** | `User` | Normal | This setting disables the startup boost feature that keeps Edge processes running in the background, saving system resources but increasing initial launch time. |
| `E153_MACHINE` | **Disable startup boost (Machine)** | `Machine` | Normal | This setting disables the startup boost feature that keeps Edge processes running in the background, saving system resources but increasing initial launch time. |
| `E154_USER` | **Hide default top sites on new tab page (User)** | `User` | Normal | This setting hides the default top sites from the new tab page, providing a cleaner new tab experience. |
| `E154_MACHINE` | **Hide default top sites on new tab page (Machine)** | `Machine` | Normal | This setting hides the default top sites from the new tab page, providing a cleaner new tab experience. |
| `E155_USER` | **Hide Adobe Acrobat subscription button (User)** | `User` | Normal | This setting hides the Adobe Acrobat subscription button in the PDF viewer, reducing third-party promotions. |
| `E155_MACHINE` | **Hide Adobe Acrobat subscription button (Machine)** | `Machine` | Normal | This setting hides the Adobe Acrobat subscription button in the PDF viewer, reducing third-party promotions. |
| `E129_USER` | **Disable the Microsoft Account Sign-In Button (User)** | `User` | Normal | This setting removes the sign-in button in Microsoft Edge. You won’t be able to sign in with a Microsoft account, and data synchronization like favorites, passwords, and settings will be disabled. |
| `E129_MACHINE` | **Disable the Microsoft Account Sign-In Button (Machine)** | `Machine` | Normal | This setting removes the sign-in button in Microsoft Edge. You won’t be able to sign in with a Microsoft account, and data synchronization like favorites, passwords, and settings will be disabled. |
| `E127_USER` | **Disable typosquatting checker for site addresses (User)** | `User` | Advanced | When entering site addresses in the Edge, they are checked for typos and corrected, so that you do not accidentally end up on a wrong (possibly malicious) website. This involves using Microsoft services to which the input must be sent. Because the risk of a fake website is high, enabling this setting is not recommended. |
| `E127_MACHINE` | **Disable typosquatting checker for site addresses (Machine)** | `Machine` | Advanced | When entering site addresses in the Edge, they are checked for typos and corrected, so that you do not accidentally end up on a wrong (possibly malicious) website. This involves using Microsoft services to which the input must be sent. Because the risk of a fake website is high, enabling this setting is not recommended. |

[↑ Back to Top](#-categories-overview)

---

## Miscellaneous

*Category contains **16** manageable settings.*

| ID | Setting / Title | Scope | Impact | Description |
| :--- | :--- | :---: | :---: | :--- |
| `M022_USER` | **Disable feedback reminders (User)** | `User` | 🟢 Recommended (Safe) |  |
| `M022_MACHINE` | **Disable feedback reminders (Machine)** | `Machine` | 🟢 Recommended (Safe) |  |
| `SHELL_PROMOTED_APPS` | **Disable automatic installation of recommended Windows Store Apps** | `Machine` | 🟢 Recommended (Safe) | Windows automatically installs suggested apps from the Windows Store in the background. This setting should be enabled if you want to prevent this from happening. |
| `M005` | **Disable tips, tricks, and suggestions while using Windows** | `Machine` | 🟢 Recommended (Safe) | From time to time, Windows displays tips and tricks, as well as suggestions for usage. This setting must be enabled if you want to prevent this from happening. |
| `M024` | **Disable Windows Media Player Diagnostics** | `Machine` | 🟢 Recommended (Safe) | Windows Media Player may send diagnostic information to Microsoft to improve services. This can be private information, so disabling it is recommended. |
| `M012` | **Disable Key Management Service Online Activation** | `Machine` | Normal | Windows periodically sends information to Microsoft to verify the activation state. Enable this setting if you want to block this online check. Doing so may have side effects when using Windows, which is why this setting is only conditionally recommended. |
| `M013` | **Disable automatic download and update of map data** | `Machine` | Normal | This setting prevents Windows from automatically downloading and automatically updating (geographic) maps. This restricts applications that need these cards and is therefore only conditionally recommended. |
| `M014` | **Disable unsolicited network traffic on the offline maps settings page** | `Machine` | Normal | Accessing the settings page for offline maps may generate network traffic that is already unwanted. Under certain conditions, this activity may be shared with Microsoft and HERE, the card manufacturer. Disabling this setting can prevent this. Since doing so disables the entire Offline Map Settings page, this setting is only conditionally recommended. |
| `M023` | **Disable installation of PC Health Check** | `Machine` | Normal | PC Health Check is an application from Microsoft to check the compatibility of the PC for Windows 11. With the KB5005463 patch, this is no longer optional, but is installed automatically. This setting prevents the installation. |
| `M026` | **Disable remote assistance connections to this computer** | `Machine` | 🟢 Recommended (Safe) | You can allow other people, such as external support technicians, or even friends and family, to access your PC so that they can help you with any maintenance or troubleshooting. This may pose a risk and should therefore be deactivated. Only for a real and verified request should this option be allowed again. |
| `M027` | **Disable remote connections to this computer** | `Machine` | 🟢 Recommended (Safe) | Disabling remote connections prevents sessions via Terminal Server or Remote Desktop. This prevents unauthorized third parties from taking over the PC. Allow remote connections only when you need to establish a verified connection.  <sb>Warning: Applying this setting blocks remote connections. Do not apply it to a remote PC (for example, in the cloud), because you will no longer be able to connect to it afterwards!</sb> |
| `M028` | **** | `Machine` | 🟢 Recommended (Safe) |  |
| `M032` | **Disable Start menu recommendations for tips, shortcuts and new apps** | `Machine` | 🟢 Recommended (Safe) | This setting disables Start menu recommendations for tips, shortcuts, new apps and similar promoted content. |
| `M033` | **Disable Start menu account notifications** | `Machine` | 🟢 Recommended (Safe) | This setting disables Microsoft account related notifications and badges in the Start menu profile area. |
| `M034` | **Disable Settings app account notifications** | `Machine` | 🟢 Recommended (Safe) | This setting disables Microsoft account related notifications and suggestions in the Windows Settings app. |
| `N001` | **Disable Network Connectivity Status Indicator** | `Machine` | Normal | Windows uses NCSI to establish connectivity to the Internet. To do this, specially defined Microsoft servers are contacted and then data transmitted. Deactivating this function prevents this from happening. Since some programs rely on this NCSI functionality, disabling it can, under certain conditions, result in interference. |

[↑ Back to Top](#-categories-overview)

---

## Mobile Devices & Phone Link

*Category contains **4** manageable settings.*

| ID | Setting / Title | Scope | Impact | Description |
| :--- | :--- | :---: | :---: | :--- |
| `D001` | **Disable access to mobile devices** | `Machine` | 🟢 Recommended (Safe) | Windows can connect to mobile devices. Data can be transferred between the mobile device (e.g. smartphone) and the Windows PC, which may reveal private data. This can be prevented with this setting. |
| `D002` | **Disable Phone Link app** | `Machine` | 🟢 Recommended (Safe) | The Phone Link application from Microsoft connects the PC to a mobile device. In doing so, data can be forwarded via Microsoft servers, which can mean a potential violation of privacy. This setting deactivates the application. |
| `D003` | **Disable showing suggestions for using mobile devices with Windows** | `Machine` | 🟢 Recommended (Safe) | Windows displays information on the use of mobile devices with the PC. These notifications can be deactivated with this setting. |
| `D104` | **Disable connecting the PC to mobile devices** | `Machine` | 🟢 Recommended (Safe) | This setting prevents Windows from connecting to mobile devices, in particular smartphones, and thus prevents data exchange, which can jeopardize the privacy and security of the PC under certain circumstances. |

[↑ Back to Top](#-categories-overview)

---

## Office & Outlook

*Category contains **22** manageable settings.*

| ID | Setting / Title | Scope | Impact | Description |
| :--- | :--- | :---: | :---: | :--- |
| `OFFICE_TELEMETRY` | **Disable telemetry for Microsoft Office** | `Machine` | 🟢 Recommended (Safe) | Microsoft Office transmits a lot of telemetry information to Microsoft. User data is collected and transmitted. To reduce data outflow, this option should be set. |
| `F014` | **Disable diagnostic data submission** | `Machine` | 🟢 Recommended (Safe) | Microsoft Office sends diagnostic information to Microsoft. This may include user-related information. This option can be used to disable both required and optional submissions. |
| `F015` | **Disable participation in the Customer Experience Improvement Program** | `Machine` | 🟢 Recommended (Safe) | Users can participate in the Customer Experience Improvement Program. In doing so, information about usage behavior is transmitted to Microsoft. Among other things, the IP address of the computer is transmitted. With this option, participation and thus transmission of data can be deactivated. |
| `F016` | **Disable the display of LinkedIn information** | `Machine` | 🟢 Recommended (Safe) | Microsoft Office can automatically obtain and display information from the LinkedIn network about its own contacts. Information is transmitted to LinkedIn servers in order to provide the data. This data transmission can be prevented with this setting. |
| `F001` | **Disable inline text prediction in mails** | `Machine` | 🟢 Recommended (Safe) | When writing mail, Microsoft Outlook can suggest text suggestions for completing a sentence. For this purpose, information is transmitted to a cloud service in order to determine the text suggestion. This can lead to unwanted information outflows and should therefore be disabled. |
| `F003` | **Disable logging for Microsoft Office Telemetry Agent** | `Machine` | 🟢 Recommended (Safe) | Companies can use a special telemetry agent for Microsoft Office to collect statistical information about the runtime of Office applications and to evaluate it in the company by administrators or authorized persons. This function can be deactivated for the local computer. |
| `F004` | **Disable upload of data for Microsoft Office Telemetry Agent** | `Machine` | 🟢 Recommended (Safe) | Companies can use a special telemetry agent for Microsoft Office to collect statistical information about the runtime of Office applications and to evaluate it in the company by administrators or authorized persons. The storage of this data on a data storage provided by the company can hereby be deactivated for the local computer. |
| `F005` | **Obfuscate file names when uploading telemetry data** | `Machine` | 🟢 Recommended (Safe) | Companies can use a special telemetry agent for Microsoft Office to collect statistical information about the runtime of Office applications and to evaluate it in the company by administrators or authorized persons. File names are usually transmitted in plain text and can contain sensitive information. This setting enables the obfuscation of these file names, making it more difficult to draw conclusions. |
| `F007` | **Disable Microsoft Office surveys** | `Machine` | 🟢 Recommended (Safe) | Microsoft may from time to time conduct surveys when using Office products to receive feedback from users. Under certain circumstances, this may be personal data. |
| `F008` | **Disable feedback to Microsoft** | `Machine` | 🟢 Recommended (Safe) | Microsoft provides ways to send feedback from the Office products. These can be deactivated with this setting. |
| `F009` | **Disable Microsoft's feedback tracking** | `Machine` | 🟢 Recommended (Safe) | If feedback on Office products is transmitted to Microsoft, an e-mail address can also be transmitted that allows Microsoft to ask questions. If this is not to be done, then the setting should be activated. |
| `F018` | **Set diagnostic data level to minimum (Neither)** | `User` | 🟢 Recommended (Safe) |  |
| `F019` | **Hide privacy settings notification on first run** | `Machine` | 🟢 Recommended (Safe) | When Microsoft 365 applications are first launched, a privacy notification may appear prompting users to review privacy settings. This setting suppresses that notification to avoid interruption. |
| `F020` | **Disable the Office first run movie** | `User` | 🟢 Recommended (Safe) | When Office is started for the first time, it plays an introductory video about signing in to Office. This video is retrieved from Microsoft over the internet. This setting suppresses the video and the connection that goes with it. |
| `OUT_HIDE_TOGGLE` | **** | `Machine` | 🟢 Recommended (Safe) | Classic Outlook shows a toggle for switching to the new Outlook. Unlike classic Outlook, the new Outlook synchronizes email accounts from other providers (IMAP, POP) via Microsoft's cloud and stores the access credentials there. This setting hides the toggle so that the switch cannot happen by accident. A new Outlook that is already installed is not affected. |
| `OUT_BLOCK_MIGRATE` | **Disable automatic migration to the new Outlook** | `Machine` | 🟢 Recommended (Safe) | Microsoft is gradually switching users of classic Outlook to the new Outlook automatically. The new Outlook synchronizes email accounts from other providers (IMAP, POP) via Microsoft's cloud and stores the access credentials there. This setting blocks the automatic switch, so classic Outlook remains in use. |
| `F006` | **Disable automatic receipt of updates** | `Machine` | Normal | This setting determines whether Microsoft Office sends diagnostic data to Microsoft and then transfers small error corrections back to the computer. These updates improve the stability of Microsoft Office, so disabling the setting is only conditionally recommended. |
| `F010` | **Disable connected experiences in Office** | `Machine` | Normal | Connected experiences in Microsoft Office provide suitable text, graphics, layouts, and other tools. To do this, the data you enter must be analyzed, which may take place in the Microsoft cloud. Enable this setting to prevent this information from being transmitted. Because this limits the use of Office, this setting is recommended with reservations. |
| `F011` | **Disable connected experiences with content analytics** | `Machine` | Normal | Connected experiences in Microsoft Office provide suitable text, graphics, layouts, and other tools. To do this, the data you enter must be analyzed, which may take place in the Microsoft cloud. Enable this setting to prevent this information from being transmitted. Because this limits the use of Office, this setting is recommended with reservations. |
| `F012` | **Disable online content downloading for connected experiences** | `Machine` | Normal | Connected experiences in Microsoft Office provide suitable text, graphics, layouts, and other tools. To do this, the data you enter must be analyzed, which may take place in the Microsoft cloud. Enable this setting to prevent this information from being transmitted. Because this limits the use of Office, this setting is recommended with reservations. |
| `F013` | **Disable optional connected experiences in Office** | `Machine` | Normal | Connected experiences in Microsoft Office provide suitable text, graphics, layouts, and other tools. To do this, the data you enter must be analyzed, which may take place in the Microsoft cloud. Enable this setting to prevent this information from being transmitted. Because this limits the use of Office, this setting is recommended with reservations. |
| `F021` | **Disable signing in to Office** | `Machine` | Normal | Office offers to sign in with a Microsoft account or an organizational account so that documents and settings can be synchronized with the cloud. This setting blocks both types of sign-in, so Office no longer establishes a connection for this purpose. Please note that features which require a sign-in, such as OneDrive or Microsoft 365, will then no longer be available. |

[↑ Back to Top](#-categories-overview)

---

## Privacy & Tracking

*Category contains **25** manageable settings.*

| ID | Setting / Title | Scope | Impact | Description |
| :--- | :--- | :---: | :---: | :--- |
| `P001` | **Disable sharing of handwriting data** | `Machine` | 🟢 Recommended (Safe) |  |
| `P002` | **Disable sharing of handwriting error reports** | `Machine` | 🟢 Recommended (Safe) |  |
| `P003` | **Disable Inventory Collector** | `Machine` | 🟢 Recommended (Safe) | Inventory Collector is primarily used in company networks and enables an overview of installed applications, devices and system information of all computers in the network. If you don't need such an overview of all computers in your network, then deactivate this function. |
| `P004` | **Disable camera in logon screen** | `Machine` | 🟢 Recommended (Safe) | Windows offers the possibility to operate the camera App from a locked PC directly from the locked screen. If you are unsure who uses your PC during your absence, then deactivate this function. |
| `PRIV_AD_ID_USER` | **Disable and reset Advertising ID and info (User)** | `User` | 🟢 Recommended (Safe) | Windows creates a commercial ID to show you advertisements based on your installed and used apps, and your browsing history. These advertisements can also be displayed in non-Microsoft apps. |
| `PRIV_AD_ID_MACHINE` | **Disable and reset Advertising ID and info (Machine)** | `Machine` | 🟢 Recommended (Safe) | Windows creates a commercial ID to show you advertisements based on your installed and used apps, and your browsing history. These advertisements can also be displayed in non-Microsoft apps. |
| `PRIV_KEYSTROKES` | **Disable transmission of typing information** | `Machine` | 🟢 Recommended (Safe) | Windows transfers data of your writing habits. Which data this is specifically and to what extent they are anonymous is unclear at this point. |
| `P026` | **Disable advertisements via Bluetooth** | `Machine` | 🟢 Recommended (Safe) | Windows can receive and transmit advertisements via Bluetooth, provided it’s near a compatible transmitter or receiver (normally circa 15-40 meters, up to 250 meters with modern devices). At the same time, additional information for optimizing advertisements can be exchanged. Disable this setting if you want to turn this feature off. |
| `P027` | **Disable the Windows Customer Experience Improvement Program** | `Machine` | 🟢 Recommended (Safe) | the Windows Customer Experience Improvement Program collects information about hardware configuration and the use of software and services, in order to compile user trends and patterns. According to Microsoft, no personal information such as names or addresses is included. We recommend disabling this setting. |
| `P028` | **Disable backup of text messages into the cloud** | `Machine` | 🟢 Recommended (Safe) | Text messages saved on the device can also be saved on external servers (e.g. by Microsoft) and restored later, should this be necessary. This setting must be enabled to prevent saving messages outside your own server. |
| `P064` | **Disable suggestions in the timeline** | `Machine` | 🟢 Recommended (Safe) | Windows occasionally displays advertisements in the Windows Explorer timeline (e.g. for OneDrive). Setting this setting suppresses these pop-ups. |
| `P065` | **Disable suggestions in Start** | `Machine` | 🟢 Recommended (Safe) | Windows occasionally displays ads in the Start menu (e.g. for new apps). Setting this setting suppresses these pop-ups. |
| `P066` | **Disable tips, tricks, and suggestions when using Windows** | `Machine` | 🟢 Recommended (Safe) | Windows occasionally displays tips, tricks, and suggestions in the Notification Pane and Info Center. Setting this setting suppresses these pop-ups. |
| `P067` | **Disable showing suggested content in the Settings app** | `Machine` | 🟢 Recommended (Safe) | Windows occasionally displays suggestions in system settings. Setting this setting suppresses these pop-ups. |
| `P070` | **Disable the possibility of suggesting to finish the setup of the device** | `Machine` | 🟢 Recommended (Safe) | This option disables the occasional display of notices when you start using Microsoft services such as Windows Hello or OneDrive and the associated prompt to create a Microsoft account. |
| `P069` | **Disable Windows Error Reporting** | `Machine` | 🟢 Recommended (Safe) | In the event of fatal failures in applications or system components, Windows creates an error report and uploads it to Microsoft servers. This may include personal information due to the memory dump and should therefore be disabled. |
| `P095` | **Disable cloud consumer account state content** | `Machine` | 🟢 Recommended (Safe) | Windows uses the state of your Microsoft account to show account related content in the Start menu, in the Settings app and in notifications - for example advertising for Microsoft services. The account state is queried online for this purpose. This setting turns off that content. |
| `P096` | **Limit crash dump collection** | `Machine` | 🟢 Recommended (Safe) | If you have agreed to send optional diagnostic data, Windows Error Reporting may transmit complete memory dumps and heap dumps. These can contain anything that was in memory at the moment of the crash, including personal data. This setting limits the transmission to kernel mini dumps and user mode triage dumps. |
| `P009` | **Disable biometric features (Windows Hello fingerprint, face, iris)** | `Machine` | Normal |  |
| `P010` | **Disable app notifications** | `Machine` | Normal | When deactivating this function, apps can no longer display notifications on the tiles, the locked screen or desktop. For apps that post reminders, this may not be the best solution. |
| `P015` | **Disable access to the browser’s language list** | `Machine` | Normal | Access to the language list of the browser enables websites to display local contents. |
| `P068` | **Disable text suggestions when typing on the software keyboard** | `Machine` | Normal | With these settings, text suggestions can be deactivated when typing on the software keyboard. Data can be loaded from the Internet to predict the words. If you want to avoid this, you should activate this setting. This means that typing on this keyboard can also take longer. |
| `P097` | **Disable Microsoft consumer features** | `Machine` | Normal | The Microsoft consumer features install suggested apps automatically, show personalized recommendations and display notifications about your Microsoft account. All of this is obtained from Microsoft over the internet. This setting turns these features off. According to Microsoft the policy only takes effect on the Enterprise and Education editions. |
| `P099` | **Disable display of office.com files in Explorer** | `Machine` | Normal | On its home page, Windows Explorer shows recently used, favorite and recommended files from office.com. To do so it retrieves metadata about your cloud files from Microsoft. This setting prevents both the query and the display. The policy is not available on the Home edition of Windows. |
| `P016` | **Disable sending URLs from apps to Windows Store** | `Machine` | Advanced | Windows analyzes the websites you access from your apps and sends this information onto the Windows Store. This feature can potentially give you more security but, at the same time, it sends data about your behavior on apps to Microsoft. Deactivate this feature if you don't want this. |

[↑ Back to Top](#-categories-overview)

---

## Security & Network

*Category contains **18** manageable settings.*

| ID | Setting / Title | Scope | Impact | Description |
| :--- | :--- | :---: | :---: | :--- |
| `S001` | **Disable password reveal button** | `Machine` | 🟢 Recommended (Safe) | If you log on to Windows, you can display the entered password while clicking on the eye symbol for a couple of seconds to check for correctness. There is a risk that somebody might peer over your shoulder while doing that. If you don't want to take this risk, then deactivate this function. |
| `S002` | **Disable user steps recorder** | `Machine` | 🟢 Recommended (Safe) | Steps recorder is used to record everything you do on your computer automatically (incl. writing in elements you have clicked on and screenshots of each click motion). The finished record can help a support specialist to solve a problem on a PC. If you don't need this function, then deactivate it to enhance your security. |
| `S003` | **Disable telemetry** | `Machine` | 🟢 Recommended (Safe) | Microsoft collects information about your computer, installed programs, and possible problems in Windows. Error reports are also sent to Microsoft. Disable this function if you don't want Microsoft to have this information.  <u>Note:</u> According to user reports, disabling this setting may result in problems with registration of the XBOX program. |
| `S015` | **Disable WiFi Sense for all users** | `Machine` | 🟢 Recommended (Safe) | WiFi sense connects automatically to public wifi hotspots which can not always guarantee security. In addition, Windows shares your wifi password of your home network with Facebook friends, Skype and Outlook.com contacts. For this, your personal wifi password will be stored in a Microsoft server. |
| `S006` | **Disable WiFi Sense for user** | `Machine` | 🟢 Recommended (Safe) | WiFi sense connects automatically to public wifi hotspots which can not always guarantee security. In addition, Windows shares your wifi password of your home network with Facebook friends, Skype and Outlook.com contacts. For this, your personal wifi password will be stored in a Microsoft server. |
| `S007` | **Disable WiFi Sense of my contacts** | `Machine` | 🟢 Recommended (Safe) | WiFi sense connects automatically to wifis of your contacts which can not always guarantee security. In addition, Windows shares your wifi password of your home network with Facebook friends, Skype and Outlook.com contacts. For this, your personal WiFi password will be stored in a Microsoft server. |
| `S008` | **Disable Internet access of Windows Media Digital Rights Management (DRM)** | `Machine` | Normal | Certain music and video files have a so-called DRM protection, which ensures that these files can only be played on your computer or restricts the amount of copies made. If you don't own DRM protected files, then deactivate this function, otherwise it is possible that you won't be able to use these files anymore. |
| `S009` | **Disable app access to wireless connections** | `Machine` | Normal | When this feature is disabled, apps will no longer have access to wireless connections and will not be able to enable or disable them on their own. This may limit some apps in their function or stop working at all. |
| `S010` | **Disable app access to loosely coupled devices** | `Machine` | Normal | If this feature is disabled, apps must not establish wireless connections that were not previously authorized (e.g. beacons). This may limit some apps in their function or stop working at all. |
| `S116` | **Disable NFC (Near Field Communication)** | `Machine` | Normal | Disables the NFC Secure Element Manager service (SEMgrSvc), preventing NFC-based communication. A system reboot is required for this change to take effect. |
| `S117` | **Disable wireless display (Miracast/WiDi)** | `Machine` | Normal | Prevents this PC from being discovered or projected to as a wireless display (Miracast/WiDi). Disabling wireless display protocols helps reduce the attack surface. |
| `S118` | **Disable mobile broadband (cellular/WWAN)** | `Machine` | Normal | Disables the Windows WWAN AutoConfig service, which manages mobile broadband (cellular) connections. Note: This setting only has an effect if a mobile broadband adapter is present in the system. |
| `S119` | **Disable WiFi Direct** | `Machine` | Normal | Disables the WiFi Direct Connection Manager service (WFDSConMgrSvc), preventing WiFi Direct peer-to-peer connections. A system reboot is required for this change to take effect. Features that build on WiFi Direct will then no longer be available, for example wireless displays (Miracast), Nearby Sharing and printing via WiFi Direct; depending on the WLAN adapter, the mobile hotspot may stop working as well. |
| `S120` | **Restrict Bluetooth pairing** | `Machine` | Normal | Restricts Bluetooth functionality via Group Policy, preventing new device pairing. This helps reduce the wireless attack surface. |
| `S012` | **Disable Microsoft SpyNet membership** | `Machine` | Normal | As soon as Microsoft Defender recognizes a threat caused by a change in files on your computer, this information can be sent to Microsoft for analysis. This is part of a so-called SpyNet membership. If you do not want this, deactivate this option. |
| `S013` | **Disable submitting data samples to Microsoft** | `Machine` | Normal | As soon as Microsoft Defender recognizes a possible threat, samples of data can be sent to Microsoft for analysis. If you do not want to send sample data, deactivate this option. |
| `S014` | **Disable reporting of malware infection information** | `Machine` | Normal | If Microsoft Defender or another security program finds an infection on your computer caused by malware, this information will be sent to Microsoft. If you do not want this, deactivate this option. |
| `S011` | **Disable Microsoft Defender** | `Machine` | Advanced | Deactivation not recommended! Microsoft Defender is the built-in Windows antivirus solution. Only deactivate this function if you use another regularly updated antivirus solution. |

[↑ Back to Top](#-categories-overview)

---

## Synchronization

*Category contains **7** manageable settings.*

| ID | Setting / Title | Scope | Impact | Description |
| :--- | :--- | :---: | :---: | :--- |
| `Y001` | **Disable synchronization of all settings** | `Machine` | 🟢 Recommended (Safe) | If you log in to Windows using a Microsoft account, you can create identical settings for all your Windows 10 devices under that account using synchronization. Your settings are synchronized with the Microsoft server and saved there. If you deactivate this setting, the entire synchronization of all further settings in this category will be deactivated too, regardless of their individual settings. |
| `Y002` | **Disable synchronization of design settings** | `Machine` | 🟢 Recommended (Safe) | If you log in to Windows using a Microsoft account, you can create identical settings for all your Windows 10 devices under that account using synchronization. Your settings are synchronized with the Microsoft server and saved there. If you do not want this then simply deactivate this setting. |
| `Y003` | **Disable synchronization of browser settings** | `Machine` | 🟢 Recommended (Safe) | If you log in to Windows using a Microsoft account, you can create identical settings for all your Windows 10 devices under that account using synchronization. Your settings are synchronized with the Microsoft server and saved there. If you do not want this then simply deactivate this setting. |
| `Y004` | **Disable synchronization of credentials (passwords)** | `Machine` | 🟢 Recommended (Safe) | If you log in to Windows using a Microsoft account, you can create identical settings for all your Windows 10 devices under that account using synchronization. Your settings are synchronized with the Microsoft server and saved there. If you do not want this then simply deactivate this setting. |
| `Y005` | **Disable synchronization of language settings** | `Machine` | 🟢 Recommended (Safe) | If you log in to Windows using a Microsoft account, you can create identical settings for all your Windows 10 devices under that account using synchronization. Your settings are synchronized with the Microsoft server and saved there. If you do not want this then simply deactivate this setting. |
| `Y006` | **Disable synchronization of accessibility settings** | `Machine` | 🟢 Recommended (Safe) | If you log in to Windows using a Microsoft account, you can create identical settings for all your Windows 10 devices under that account using synchronization. Your settings are synchronized with the Microsoft server and saved there. If you do not want this then simply deactivate this setting. |
| `Y007` | **Disable synchronization of advanced Windows settings** | `Machine` | 🟢 Recommended (Safe) | If you log in to Windows using a Microsoft account, you can create identical settings for all your Windows 10 devices under that account using synchronization. Your settings are synchronized with the Microsoft server and saved there. If you do not want this then simply deactivate this setting. |

[↑ Back to Top](#-categories-overview)

---

## Taskbar & Start Menu

*Category contains **7** manageable settings.*

| ID | Setting / Title | Scope | Impact | Description |
| :--- | :--- | :---: | :---: | :--- |
| `M019_USER` | **Disable news and interests in the task bar (User)** | `User` | 🟢 Recommended (Safe) | This setting allows you to disable the display of news and interesting topics in the taskbar if you do not want them to be displayed. In the active state, data from Microsoft Bing services is retrieved at regular intervals and thus Internet connections are established. |
| `M019_MACHINE` | **Disable news and interests in the task bar (Machine)** | `Machine` | 🟢 Recommended (Safe) | This setting allows you to disable the display of news and interesting topics in the taskbar if you do not want them to be displayed. In the active state, data from Microsoft Bing services is retrieved at regular intervals and thus Internet connections are established. |
| `M016` | **Disable search box in task bar** | `Machine` | 🟢 Recommended (Safe) | You can disable the search box in the task bar with this setting, if you do not want to have it displayed. |
| `M015` | **Disable People icon in the taskbar** | `Machine` | Normal | Windows can display the People icon in the taskbar. Disable this setting to prevent this. |
| `M017_USER` | **** | `User` | Normal |  |
| `M017_MACHINE` | **** | `Machine` | Normal |  |
| `M021` | **Disable widgets in Windows Explorer** | `Machine` | 🟢 Recommended (Safe) | Windows 11 introduced widgets that can display personalized information. For this purpose, information is exchanged with corresponding servers on the Internet. If you want to deactivate this option in Windows Explorer, then you have to activate this setting. |

[↑ Back to Top](#-categories-overview)

---

## Telemetry & Diagnostics

*Category contains **11** manageable settings.*

| ID | Setting / Title | Scope | Impact | Description |
| :--- | :--- | :---: | :---: | :--- |
| `TEL_DIAGTRACK` | **Disable Connected User Experiences and Telemetry Service** | `Service` | 🟢 Recommended (Safe) | The DiagTrack service collects diagnostic events and transmits them continuously to Microsoft. |
| `TEL_DMWAP` | **Disable WAP Push Message Routing Service** | `Service` | 🟢 Recommended (Safe) | Routes telemetry and diagnostic packets on behalf of Windows telemetry components. |
| `TEL_CEIP` | **Disable Customer Experience Improvement Program (CEIP)** | `Machine` | 🟢 Recommended (Safe) | Windows CEIP gathers anonymous information on how users interact with Windows components. |
| `TEL_FEEDBACK` | **Disable Windows Feedback Notification Prompts** | `Machine` | 🟢 Recommended (Safe) | Prevents Windows from periodically displaying pop-ups asking for ratings and feedback. |
| `TEL_CRASHDUMP` | **Restrict Windows Crash Dump & Error Reporting Telemetry** | `Machine` | 🟢 Recommended (Safe) | Restricts Windows Error Reporting (WER) from automatically transmitting crash dumps to the cloud. |
| `TEL_DIAGDATA` | **Disable application telemetry** | `Machine` | 🟢 Recommended (Safe) | By deactivating this function, Microsoft will not send telemetry data, i.e. usage data of programs, crashes, your entry behavior and similar are no longer sent to Microsoft. |
| `PRIV_TAILORED_USER` | **Disable diagnostic data from customizing user experiences (User)** | `User` | 🟢 Recommended (Safe) | Microsoft can record diagnostic data from your computer and evaluate it in order to improve your use of Windows. While doing so, a large amount of such data will be compiled and transmitted. Disable this feature if you want to stop this from happening. |
| `PRIV_TAILORED_MACHINE` | **Disable diagnostic data from customizing user experiences (Machine)** | `Machine` | 🟢 Recommended (Safe) | Microsoft can record diagnostic data from your computer and evaluate it in order to improve your use of Windows. While doing so, a large amount of such data will be compiled and transmitted. Disable this feature if you want to stop this from happening. |
| `U006` | **Disable diagnostic log collection** | `Machine` | 🟢 Recommended (Safe) | Diagnostic logs are created to collect information about problems on the device. These are sent when diagnostic data delivery is enabled. This option prevents the creation of log files. |
| `U007` | **Disable downloading of OneSettings configuration settings** | `Machine` | 🟢 Recommended (Safe) | Microsoft's OneSettings service allows automatic download of configuration settings to address problems on the machine. This involves establishing a connection to Microsoft servers and possibly exchanging machine-related information. This option can be used to disable this. |
| `U008` | **Do not send device name in diagnostic data** | `Machine` | 🟢 Recommended (Safe) | This setting prevents Windows from including the device name in diagnostic data sent to Microsoft. |

[↑ Back to Top](#-categories-overview)

---

## Windows Explorer

*Category contains **5** manageable settings.*

| ID | Setting / Title | Scope | Impact | Description |
| :--- | :--- | :---: | :---: | :--- |
| `M006` | **Disable occasionally showing app suggestions in Start menu** | `Machine` | 🟢 Recommended (Safe) | The Start menu displays suggestions for apps from the Windows Store at irregular intervals. When you click these apps, they are automatically installed and made available on the system. This setting must be enabled if you want to avoid this. |
| `M011` | **** | `Machine` | Normal | Windows saves recently opened items in special lists so that these may be quickly started over the Start menu or the taskbar. Enable this setting if you wish to hide these items or the file names. |
| `M010` | **Disable ads in Windows Explorer/OneDrive** | `Machine` | Normal | Advertizing pop-ups may appear when connecting the Windows Explorer and integration of OneDrive, when these ads come directly from Microsoft. This setting will block such advertisements.   <u>Note:</u> Disabling this setting will also disable all other notices from Microsoft OneDrive. We therefore recommend this setting with reservation. |
| `O003` | **Disable OneDrive access to network before login** | `Machine` | Normal | OneDrive checks for updates or synchronizes files before users log in. You can use this setting to disable network access and prevent this from happening. |
| `O001` | **Disable Microsoft OneDrive** | `Machine` | Advanced | If you do not want to use Microsoft’s Cloud storage service OneDrive then you can deactivate it here. |

[↑ Back to Top](#-categories-overview)

---

## Windows Update

*Category contains **14** manageable settings.*

| ID | Setting / Title | Scope | Impact | Description |
| :--- | :--- | :---: | :---: | :--- |
| `WU_AUTO_REBOOT` | **Disable Automatic Reboot with Logged-On Users** | `Machine` | 🟢 Recommended (Safe) | Prevents Windows from rebooting automatically while a user is currently logged on. |
| `A004_USER` | **Disable automatic Windows Updates (User)** | `User` | Advanced | Deactivating is not recommended! With this you deactivate the automatic installation of Windows Updates. Security leaks will not be tackled automatically. |
| `A004_MACHINE` | **Disable automatic Windows Updates (Machine)** | `Machine` | Advanced | Deactivating is not recommended! With this you deactivate the automatic installation of Windows Updates. Security leaks will not be tackled automatically. |
| `A005` | **Disable Windows Updates for other products (e.g. Microsoft Office)** | `Machine` | Advanced | Deactivation is not recommended! The automatic update of many products, like e.g. Microsoft Office, is prevented by this. |
| `P007_USER` | **Disable optional updates (including preview updates) (User)** | `User` | 🟢 Recommended (Safe) | Windows 10 (from version 20H2 onward) allows optional updates, including preview updates, to be installed. These updates may include new features or fixes that have not yet been fully tested. Disable this setting to prevent optional updates from being installed automatically. |
| `P007_MACHINE` | **Disable optional updates (including preview updates) (Machine)** | `Machine` | 🟢 Recommended (Safe) | Windows 10 (from version 20H2 onward) allows optional updates, including preview updates, to be installed. These updates may include new features or fixes that have not yet been fully tested. Disable this setting to prevent optional updates from being installed automatically. |
| `WU_DELIVERY_OPT_USER` | **Disable Windows Update via peer-to-peer (User)** | `User` | 🟢 Recommended (Safe) | Windows Updates do not have to be downloaded only from Microsoft servers, but can also be downloaded from PCs in your network or the Internet. This often speeds up the process. It is a disadvantage that update data is sent from your computer too so that upload speeds can be reduced. If you don't want this, then deactivate this function. |
| `WU_DELIVERY_OPT_MACHINE` | **Disable Windows Update via peer-to-peer (Machine)** | `Machine` | 🟢 Recommended (Safe) | Windows Updates do not have to be downloaded only from Microsoft servers, but can also be downloaded from PCs in your network or the Internet. This often speeds up the process. It is a disadvantage that update data is sent from your computer too so that upload speeds can be reduced. If you don't want this, then deactivate this function. |
| `W011` | **Disable updates to the speech recognition and speech synthesis modules** | `Machine` | 🟢 Recommended (Safe) | Windows periodically reviews the availability of new speech recognition and synthesis modules that are used for converting text to speech and vice versa. These are then downloaded automatically in the background. Disable this setting if you want to prevent this from happening. |
| `W004` | **Activate deferring of upgrades** | `Machine` | Normal | Upgrades (not security updates) can be pushed back for some months to install them at a date of your choosing. |
| `W005` | **Disable automatic downloading manufacturers' apps and icons for devices** | `Machine` | Normal | Many device manufacturers provide Windows with special programs that enable their devices to be used more easily or even used at all. This setting can prevent the downloading of such programs. |
| `W010` | **Disable automatic driver updates through Windows Update** | `Machine` | Normal | Hardware drivers will be automatically updated with Windows Updates. Often, the drivers of the hardware producers are more current and more specific. Gamers may profit more from the use of hardware drivers. If you want to update hardware drivers yourself at your preferred time, then deactivate this function. |
| `W009` | **Disable automatic app updates through Windows Update** | `Machine` | Normal | Apps will be automatically updated with Windows Updates. If you want to update the apps through Windows Store yourself at your preferred time, then deactivate this function. |
| `P017` | **Disable Windows dynamic configuration and update rollouts** | `Machine` | Normal |  |

[↑ Back to Top](#-categories-overview)

---

## 📄 License

PrivatizeWin is free, open-source software licensed under the [MIT License](LICENSE).
