# Client/Server/Tools on Windows Platform
<details><summary>Click to expand</summary>

### 1. System Packages
Package | Download Link
------------ | -------------
XAMPP | https://downloadsapachefriends.global.ssl.fastly.net/8.1.6/xampp-windows-x64-8.1.6-0-VS16-installer.exe?from_af=true
Python 3.7 | https://www.python.org/ftp/python/3.7.9/python-3.7.9-amd64.exe
Git | https://github.com/git-for-windows/git/releases/download/v2.32.0.windows.1/Git-2.32.0-64-bit.exe
vcpkg | https://github.com/microsoft/vcpkg
DirectX SDK | https://www.microsoft.com/en-us/download/details.aspx?id=6812

**Notes**: 
- Clone vcpkg to **C:\Program Files\vcpkg** (Used as a default path in further instructions)
- Install DirectX SDK to **C:\Program Files (x86)\Microsoft DirectX SDK (June 2010)** (Suggested by default)

### 2. Git Repository

### 3. Install dependencies
1. Copy **portfile.cmake** located in the **Files** folder to C:\Program Files\vcpkg\ports\g3log.
2. Navigate to vcpkg directory *(C:\Program Files\vcpkg)* using command promt started with admin rights
3. Run the following commands:
- bootstrap-vcpkg.bat
- vcpkg integrate install
- vcpkg install --triplet=x64-windows-static devil boost python3 pybind11 lzo fmt spdlog xxHash lz4 libmariadb sdl2 g3log glm OpenSSL freetype tweeny cxxopts libffi expat IlmBase WTL crashpad

### 4. Python Packages
Navigate to Source/Server directory using command promt and execute the following command:
- pip install -r PythonDependencies.txt

### 5. CMake Project Configuration
Option | Description
------------ | -------------
Where is the source code | Path to the git clone directory.
Where to build the binaries | Path to a directory that will contain project build files.
Generator | Visual Studio version.
Platform | Platform on which application will be based. Default is x64.
Toolchain | Toolchain which links the libraries.

1. Configure source code and build paths (Use source-code-path/Build as build path).
<img src="Source/Server/SetupFiles/Images/Path%20Configuration.png" width="35%"/>

2. Configure the project settings. Select latest visual studio version, x32 platform and tick **Specify toolchain for cross-platform compiling**.
<img src="Source/Server/SetupFiles/Images/Configuration%20Setup.png" width="35%"/>
<img src="Source/Server/SetupFiles/Images/Toolchain%20Setup.png" width="35%"/>

3. The project will proceed with configuring phase. Expect *Windows static builds are required* error.
4. Set **VCPKG_TARGET_TRIPLET** to **x64-windows-static** to resolve the issue with windows static builds.
5. Configure the project again and generate it.

### 6. SQL Setup
Do not forget to start up SQL server on XAMPP/your preferred application.

1. Create an SQL user metin2-VerySecurePassword and assign priviliges to it.
2. Install SQL tables using the templates located in **Source/Server/SetupFiles/SQL**.

### 7. Serverfiles Setup
CMake Prefixes | Description
------------ | -------------
SERVER_BUILD_VARIANT | Select build type for server (debug/release/minsizerel/relwithdebinfo).
SERVER_CONFIG_FILEPATH | Path to the serverfiles config file.
CMAKE_INSTALL_PREFIX | Path to the serverfiles generation directory.

- Once everything is setup build INSTALL in Visual Studio Solution.
- Copy **start.bat** from **Source/Server/SetupFiles/Services** to parent directory of your servefiles directory.
- Edit the path to serverfiles directory and start up the serverfiles services.
- Control serverfiles services via [website](https://mega.nz/file/ZNNBGRZA#ViEXBLuC55rzJQNt3qsrWfcNg207V1FPHNtUMUmKpB8) that you host on your local apache server.

### 8. Repository/Dependencies Update Management
Navigate to git repository directory using command promt and execute the following command:
- git pull && vcpkg upgrade --no-dry-run


</details>
</details>
