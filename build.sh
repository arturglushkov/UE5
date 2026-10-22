#!/bin/zsh
# NOAR: one-command editor build. Writes Saved/BuildLogs/ so Claude can read the result itself.
cd "$(dirname "$0")"
UE="/Users/Shared/Epic Games/UE_5.8"
mkdir -p Saved/BuildLogs
"$UE/Engine/Build/BatchFiles/Mac/Build.sh" NOAREditor Mac Development -Project="$PWD/NOAR.uproject" -WaitMutex 2>&1 | tee Saved/BuildLogs/build.log
cp "$HOME/Library/Application Support/Epic/UnrealBuildTool/Log.txt" Saved/BuildLogs/UBT_Log.txt 2>/dev/null
cp "$HOME/Library/Application Support/Epic/UnrealBuildTool/Log_UHT.txt" Saved/BuildLogs/UHT_Log.txt 2>/dev/null
ls -t "$HOME/Library/Logs/Unreal Engine/"*/UnrealHeaderTool*.log 2>/dev/null | head -1 | xargs -I{} cp {} Saved/BuildLogs/UHT_Log2.txt 2>/dev/null
echo ""; echo "=== done: tell Claude 'собрал' ==="
