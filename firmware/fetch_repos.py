import os
import subprocess
import json


def clone_or_update_repo(
    repo_url, path, ref=None, with_submodules=False, patch_path=None
):
    import os

    if not os.path.exists(path):
        subprocess.run(["git", "clone", repo_url, path], check=True)
    else:
        subprocess.run(["git", "-C", path, "fetch"], check=True)

    if ref:
        subprocess.run(["git", "-C", path, "checkout", ref], check=True)

    if with_submodules:
        subprocess.run(
            ["git", "-C", path, "submodule", "update", "--init", "--recursive"],
            check=True,
        )

    # 应用 patch
    if patch_path:
        patch_full_path = (
            patch_path
            if os.path.isabs(patch_path)
            else os.path.join(os.getcwd(), patch_path)
        )
        # Windows checkouts can have CRLF line endings. Ignore whitespace while
        # applying the upstream patch and fail rather than build unpatched code.
        base = ["git", "-C", path, "apply", "--ignore-whitespace"]
        if subprocess.run(base + ["--check", patch_full_path], capture_output=True).returncode == 0:
            subprocess.run(base + [patch_full_path], check=True)
            print(f"Applied patch {patch_path} to {path}")
        elif subprocess.run(base + ["--reverse", "--check", patch_full_path], capture_output=True).returncode == 0:
            print(f"Patch {patch_path} already applied to {path}")
        else:
            raise RuntimeError(f"Patch {patch_path} cannot be applied to {path}")



def fetch_dependencies():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    config_path = os.path.join(script_dir, "repos.json")

    with open(config_path) as f:
        repos = json.load(f)

    for repo in repos:
        repo_path = os.path.join(script_dir, repo["path"])
        branch = repo.get("branch")
        with_submodules = repo.get("with_submodules", False)
        patch = repo.get("patch")
        if patch and not os.path.isabs(patch):
            patch = os.path.join(script_dir, patch)
        clone_or_update_repo(repo["url"], repo_path, branch, with_submodules, patch)


if __name__ == "__main__":
    fetch_dependencies()