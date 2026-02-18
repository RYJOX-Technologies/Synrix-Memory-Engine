# Sending the License Key to Joseph via Discord (Option 1: Send Separate)

## What you're doing

- **Part A:** Encrypted key (safe to send in Discord — useless without the password).
- **Part B:** Password (send separately: different app, or later in Discord after Joseph is ready).

---

## Your steps (Ryan)

### 1. Export the key to PKCS8 base64

In a terminal, from `build/windows/tools/`:

```powershell
python export_private_key_pkcs8_base64.py synrix_license_private.key > pkcs8.txt
```

(If the key file is elsewhere, use the full path.)

### 2. Encrypt it with a password

```powershell
python encrypt_key_for_transfer.py pkcs8.txt
```

- When prompted, choose a **strong password** (e.g. 16+ characters, random if possible).
- You’ll get a long base64 string printed — that’s **Part A**.

### 3. Send Part A on Discord

- Copy the **entire** base64 block (between the “Part A” lines).
- Send it to Joseph in Discord (one message or a file paste).
- You can say: “Part A – encrypted key. Password coming separately.”

### 4. Send Part B separately

- **Do not** send the password in the same Discord message or right after.
- Send the password via one of:
  - **Signal / WhatsApp** (direct message to Joseph), or
  - **Phone call / in person**, or
  - **A second Discord message** only after Joseph says he’s ready (e.g. “send password now”), so it’s not in the same thread as Part A.

### 5. Give Joseph the decrypt script

- Send him the file **`decrypt_key_for_transfer.py`** (e.g. in Discord, or he can get it from the repo if it’s there).
- Or paste the contents of `DECRYPT_INSTRUCTIONS.md` (below) so he knows how to run it.

---

## Joseph’s steps (decrypt and use)

1. Save the Part A base64 blob to a file, or have it ready to paste.
2. Install Python and run: `pip install cryptography`
3. Run the decrypt script:
   - **Option A:** `python decrypt_key_for_transfer.py` then paste Part A, press Enter, then Ctrl+Z (Windows) or Ctrl+D (Mac/Linux), then enter the password when prompted.
   - **Option B:** Put Part A in a file `part_a.txt`, then: `python decrypt_key_for_transfer.py < part_a.txt` and enter the password when prompted.
4. The script prints the PKCS8 base64 in one line. Copy it.
5. Set it in Supabase:  
   `supabase secrets set LICENSE_SIGNING_PRIVATE_KEY=<paste that one line>`
6. Delete local copies of Part A and the decrypted output.

---

## Security notes

- Part A by itself is useless; it’s encrypted with the password.
- Sending the password through a **different channel** (e.g. Signal vs Discord) reduces risk if one channel is later exposed.
- After Joseph sets the secret in Supabase, he should delete the decrypted key from his machine and any local files containing Part A or the password.
