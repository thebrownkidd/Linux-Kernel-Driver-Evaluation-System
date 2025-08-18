import subprocess
import json
import os

def run_cmd(cmd):
    try:
        result = subprocess.run(cmd, shell=True, capture_output=True, text=True)
        return result.returncode, result.stdout + result.stderr
    except Exception as e:
        return 1, str(e)

def evaluate_driver(path="mygpio.c"):
    metrics = {
        "compilation": {"success": False, "warnings": 0, "errors": 0},
        "functionality": {"loaded": False, "unloaded": False},
        "code_quality": {"style_score": 0.0},
        "overall_score": 0.0,
    }

    # Step 1: Compile
    make_cmd = f"make -C /lib/modules/$(uname -r)/build M=$(pwd) modules"
    ret, out = run_cmd(make_cmd)

    if ret == 0 and os.path.exists("mygpio.ko"):
        metrics["compilation"]["success"] = True
        metrics["compilation"]["warnings"] = out.count("warning")
        metrics["compilation"]["errors"] = out.count("error")
    else:
        metrics["compilation"]["errors"] += 1
        return metrics

    # Step 2: Style check (checkpatch.pl)
    checkpatch = "/usr/src/linux/scripts/checkpatch.pl"  # adjust path if needed
    if os.path.exists(checkpatch):
        _, style_out = run_cmd(f"perl {checkpatch} --no-tree --file {path}")
        total = style_out.count("WARNING") + style_out.count("ERROR")
        metrics["code_quality"]["style_score"] = 1.0 if total == 0 else max(0, 1 - total/10)

    # Step 3: Load driver
    ret, _ = run_cmd("sudo insmod mygpio.ko")
    if ret == 0:
        metrics["functionality"]["loaded"] = True
        run_cmd("sudo rmmod mygpio")
        metrics["functionality"]["unloaded"] = True

    # Step 4: Score
    score = 0
    if metrics["compilation"]["success"]:
        score += 40
    if metrics["functionality"]["loaded"] and metrics["functionality"]["unloaded"]:
        score += 30
    score += metrics["code_quality"]["style_score"] * 30
    metrics["overall_score"] = score

    return metrics

if __name__ == "__main__":
    result = evaluate_driver()
    print(json.dumps(result, indent=4))
