Ash
---
Ash is the "Aura Shell", the window manager and system UI for Chrome OS.
Ash uses the views UI toolkit (e.g. views::View, views::Widget, etc.) backed
by the aura native widget and layer implementations.

Ash sits below chrome in the dependency graph (i.e. it cannot depend on code
in //chrome). It has a few dependencies on //content, but these are isolated
in their own module in //ash/content. This allows targets like ash_unittests
to build more quickly.

Tests
-----
Most tests should be added to the ash_unittests target. Tests that rely on
//content should be added to ash_content_unittests, but these should be rare.

Tests can bring up most of the ash UI and simulate a login session by deriving
from AshTestBase. This is often needed to test code that depends on ash::Shell
and the controllers it owns.

Test support code (TestFooDelegate, FooControllerTestApi, etc.) lives in the
same directory as the class under test (e.g. //ash/foo rather than //ash/test).
Test code uses namespace ash; there is no special "test" namespace.

Mus+ash
----------
Ash is transitioning to use the mus window server and gpu process, found in
//services/ui. Ash continues to use aura, but aura is backed by mus. Code to
support mus is found in //ash/mus. There should be relatively few differences
between the pure aura and the aura-mus versions of ash. Ash can by run in mus
mode by passing the --mus command line flag.

Ash is also transitioning to run as a mojo service in its own process. This
means that code in chrome cannot call into ash directly, but must use the mojo
interfaces in //ash/public/interfaces.

Out-of-process Ash is referred to as "mash" (mojo ash). In-process ash is
referred to as "classic ash". Ash can run in either mode depending on the
--mash command line flag.

Prefs
-----
Ash supports both per-user prefs and device-wide prefs. These are called
"profile prefs" and "local state" to match the naming conventions in chrome. Ash
also supports "signin screen" prefs, bound to a special profile that allows
users to toggle features like spoken feedback at the login screen.

Local state prefs are loaded asynchronously during startup. User prefs are
loaded asynchronously after login, and after adding a multiprofile user. Code
that wants to observe prefs must wait until they are loaded. See
ShellObserver::OnLocalStatePrefServiceInitialized() and
SessionObserver::OnActiveUserPrefServiceChanged(). All PrefService objects exist
for the lifetime of the login session, including the signin prefs.

Pref names are in //ash/public/cpp so that code in chrome can also use the
names. Prefs are registered in the classes that use them because those classes
have the best knowledge of default values.

All PrefService instances in ash are backed by the mojo preferences service.
This means an update to a pref is asynchronous between code in ash and code in
chrome. For example, if code in chrome changes a pref value then immediately
calls a C++ function in ash, that ash function may not see the new value yet.
(This pattern only happens in the classic ash configuration; code in chrome
cannot call directly into the ash process in the mash config.)

Prefs are either "owned" by ash or by chrome browser. New prefs used by ash
should be owned by ash. See NightLightController and LogoutButtonTray for
examples of ash-owned prefs. See //services/preferences/README.md for details of
pref ownership and "foreign" prefs.

Historical notes
----------------
Ash shipped on Windows for a couple years to support Windows 8 Metro mode.
Windows support was removed in 2016.
