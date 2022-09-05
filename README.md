# skeletal-playdate
Project Skeleton for C++ Playdate Development

Build Instructions:
-------------------
- Down load the zipped GNU ARM toolchain from here [Arm GNU Toolchain Downloads](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads).
- Extract the contents of the zip file.
- Put the uncompressed folder somewhere, it will be called something like `arm-gnu-toolchain-{something}-arm-none-eabi`
- I put it alongside the Playdate SDK in my `~/Developer` folder.
- Open the `build.sh` file in this project.
- Change the `arm_toolchain_dir` variable at the top of the file to the path to the GNU ARM toolchain `bin` folder.

```
./build.sh
```
