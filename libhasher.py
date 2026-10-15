import os
import sys
import random
import pefile


def generate_junk_bytes(length):
    """Generate random non-null bytes for a PE overlay."""
    return bytes(random.randint(1, 255) for _ in range(length))


def random_section_name():
    """Generate a valid PE section name of 1-8 ASCII characters."""
    alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
    length = random.randint(4, 7)
    return "".join(random.choice(alphabet) for _ in range(length))


def obfuscate_pe(input_path, output_path):
    print(f"\n[*] Parsing target Windows PE: {input_path}")

    try:
        pe = pefile.PE(input_path)
    except Exception as e:
        print(f"[-] Error opening PE: {e}")
        return False

    try:
        print("[*] Import directory:")
        if hasattr(pe, "DIRECTORY_ENTRY_IMPORT"):
            import_count = 0

            for entry in pe.DIRECTORY_ENTRY_IMPORT:
                dll_name = (
                    entry.dll.decode("ascii", errors="replace")
                    if entry.dll
                    else "<unknown>"
                )

                for imp in entry.imports:
                    if imp.name:
                        api_name = imp.name.decode(
                            "ascii",
                            errors="replace"
                        )
                        print(f"    {dll_name}!{api_name}")
                        import_count += 1

            print(f"[*] Preserving {import_count} named imports.")
        else:
            print("[*] No import directory found.")

        # ------------------------------------------------------------
        # IMPORTANT:
        #
        # Do NOT overwrite IMAGE_IMPORT_BY_NAME strings.
        #
        # Windows needs these strings when resolving the executable's
        # imports. Replacing them with hashes makes the PE unloadable
        # unless a custom runtime import resolver is present.
        # ------------------------------------------------------------

        print("[*] Changing section names...")

        for section in pe.sections:
            old_name = section.Name.rstrip(b"\x00").decode(
                "ascii",
                errors="replace"
            )

            new_name = random_section_name()
            section.Name = new_name.encode("ascii").ljust(8, b"\x00")

            print(f"    {old_name!r} -> {new_name!r}")

        # Generate the PE while preserving its actual structures.
        raw_pe_bytes = pe.write()

        # ------------------------------------------------------------
        # Append an overlay.
        #
        # Data appended after the PE's normal image is not mapped as
        # executable memory and therefore does not interfere with the
        # normal PE loader.
        # ------------------------------------------------------------

        junk_size = random.randint(1024, 4096)

        print(
            f"[*] Appending {junk_size:,} bytes of non-executable "
            "overlay data..."
        )

        raw_pe_bytes += generate_junk_bytes(junk_size)

        with open(output_path, "wb") as f:
            f.write(raw_pe_bytes)

        print(f"[+] Written: {output_path}")

        # ------------------------------------------------------------
        # Re-open the resulting file to make sure pefile can parse it.
        # ------------------------------------------------------------

        print("[*] Validating resulting PE...")

        try:
            check = pefile.PE(output_path)

            machine = check.FILE_HEADER.Machine
            entry_point = check.OPTIONAL_HEADER.AddressOfEntryPoint

            print(f"[+] PE validation succeeded.")
            print(f"    Machine:       0x{machine:04X}")
            print(f"    Entry point:   0x{entry_point:08X}")
            print(f"    File size:     {os.path.getsize(output_path):,} bytes")

            check.close()

        except Exception as e:
            print(f"[-] Output PE failed validation: {e}")
            return False

        return True

    finally:
        pe.close()


def main():
    if len(sys.argv) >= 2:
        input_file = sys.argv[1]
    else:
        input_file = input(
            "Enter the input .exe filename to process: "
        ).strip()

    if not input_file:
        print("[-] No input file specified.")
        sys.exit(1)

    if not os.path.isfile(input_file):
        print(f"[-] File not found: {input_file}")
        sys.exit(1)

    base_name, ext = os.path.splitext(input_file)

    if ext.lower() != ".exe":
        print("[!] Warning: input does not have a .exe extension.")

    output_file = f"{base_name}_mutated.exe"

    if os.path.abspath(input_file) == os.path.abspath(output_file):
        print("[-] Input and output files must be different.")
        sys.exit(1)

    success = obfuscate_pe(input_file, output_file)

    if not success:
        sys.exit(1)

    print("\n[+] Finished successfully.")


if __name__ == "__main__":
    main()
