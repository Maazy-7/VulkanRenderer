@echo off
for %%F in (Shaders\*) do (
  if /I not "%%~xF"==".spv" (
    C:\VulkanSDK\1.4.335.0\Bin\glslc.exe "%%F" -o "%%~fF.spv"
    echo "Compiling %%F"
  )
)
pause