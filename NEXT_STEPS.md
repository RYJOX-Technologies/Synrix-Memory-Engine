# Next steps – push to GitHub

The repo is initialized and the first commit is done. Do this next:

---

## 1. Create the repo on GitHub

1. Go to **https://github.com/new**
2. **Repository name:** e.g. `synrix` or `synrix-windows-build`
3. **Private** or **Public** – your choice
4. **Do not** add a README, .gitignore, or license (you already have them)
5. Click **Create repository**

---

## 2. Add remote and push

In PowerShell, from **`C:\Users\Livew\Desktop\synrix-windows-build`**:

```powershell
git remote add origin https://github.com/YOUR_USERNAME/REPO_NAME.git
git branch -M main
git push -u origin main
```

Replace **YOUR_USERNAME** with your GitHub username and **REPO_NAME** with the repo name you chose (e.g. `synrix`).

If GitHub shows you a URL like `https://github.com/YourOrg/synrix.git`, use that:

```powershell
git remote add origin https://github.com/YourOrg/synrix.git
git branch -M main
git push -u origin main
```

---

## 3. (Optional) Ship tier zips via Releases

- Build and zip your packages locally (you already have the zips in `synrix-windows-build/packages/*.zip`).
- On GitHub: **Releases** → **Create a new release** → attach the `.zip` files.
- Do **not** commit the zips to the repo (they’re in `.gitignore`).

---

**Done:** After step 2, your code is on GitHub. Use `GITHUB_SETUP.md` for more detail.
