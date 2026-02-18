# Decrypting the license key (for Joseph)

Ryan sent you **Part A** (encrypted base64) and **Part B** (password) separately.

## 1. Save Part A

Paste the Part A blob Ryan sent into a file, e.g. `part_a.txt`. (Remove any “Part A” labels; keep only the base64.)

## 2. Install Python and cryptography

```bash
pip install cryptography
```

## 3. Run the decrypt script

If you have `decrypt_key_for_transfer.py`:

```bash
python decrypt_key_for_transfer.py part_a.txt
```

Enter the password (Part B) when prompted. The script will print the **PKCS8 base64** in one line.

## 4. Set the secret in Supabase

Copy that one line and run:

```bash
supabase secrets set LICENSE_SIGNING_PRIVATE_KEY=<paste the line here>
```

## 5. Clean up

Delete `part_a.txt` and any local copy of the decrypted key. Do not commit them.
