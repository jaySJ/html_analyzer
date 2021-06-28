cmake -G "Visual Studio 16 2019" -A x64 -S . -B "build_x64"
cd build_x64
msbuild HtmlAnalyzer.sln -m
msbuild HtmlAnalyzer.sln -m /property:Configuration=Release
