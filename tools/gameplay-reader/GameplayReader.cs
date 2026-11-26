// SPDX-License-Identifier: MIT
// Reads plaintext, uncompressed PAK entries only. No keys, decryption or game execution.
using System;
using System.IO;
using System.Reflection;
using System.Security.Cryptography;

public sealed class GameplayAssemblyResult
{
    public byte[] Bytes;
    public string Name;
    public string Version;
    public long EntryOffset;
}

public static class GameplayReader
{
    private static bool Equal(byte[] a, int offset, byte[] b)
    {
        for (int i = 0; i < b.Length; i++) if (a[offset + i] != b[i]) return false;
        return true;
    }

    private static bool IsManagedPe(byte[] bytes)
    {
        if (bytes.Length < 256 || bytes[0] != 77 || bytes[1] != 90) return false;
        int pe = BitConverter.ToInt32(bytes, 60);
        if (pe < 64 || pe > bytes.Length - 256 || BitConverter.ToUInt32(bytes, pe) != 0x4550) return false;
        int optional = pe + 24;
        ushort magic = BitConverter.ToUInt16(bytes, optional);
        int directories = magic == 0x10b ? optional + 96 : magic == 0x20b ? optional + 112 : -1;
        if (directories < 0 || directories + 120 > bytes.Length) return false;
        return BitConverter.ToUInt32(bytes, directories + 112) != 0 && BitConverter.ToUInt32(bytes, directories + 116) >= 72;
    }

    public static GameplayAssemblyResult Find(byte[] pak, long dataEnd, string temporaryDirectory)
    {
        if (dataEnd < 64 || dataEnd > pak.LongLength || pak.LongLength > 128L * 1024 * 1024) throw new InvalidDataException("INVALID_SCAN_BOUND");
        GameplayAssemblyResult found = null;
        for (int mz = 64; mz < dataEnd - 256; mz++)
        {
            if (pak[mz] != 77 || pak[mz + 1] != 90) continue;
            // Observed entry layout: 53-byte header, 11 zero bytes, then an ordinary PE.
            int header = mz - 64;
            bool prefix = true;
            for (int j = mz - 11; j < mz; j++) if (pak[j] != 0) { prefix = false; break; }
            if (!prefix) continue;
            long size = BitConverter.ToInt64(pak, header + 8);
            long unpacked = BitConverter.ToInt64(pak, header + 16);
            uint compression = BitConverter.ToUInt32(pak, header + 24);
            byte flags = pak[header + 48];
            if (size < 267 || size != unpacked || compression != 0 || (flags & 1) != 0) continue;
            if (size > dataEnd - (header + 53)) continue;
            byte[] digest;
            using (SHA1 sha = SHA1.Create()) digest = sha.ComputeHash(pak, header + 53, (int)size);
            if (!Equal(pak, header + 28, digest)) continue;
            byte[] image = new byte[(int)size - 11];
            Buffer.BlockCopy(pak, mz, image, 0, image.Length);
            if (!IsManagedPe(image)) continue;
            string candidate = Path.Combine(temporaryDirectory, Guid.NewGuid().ToString("N") + ".dll");
            try
            {
                File.WriteAllBytes(candidate, image);
                // Reads assembly identity; does not load or run the assembly.
                AssemblyName identity;
                try { identity = AssemblyName.GetAssemblyName(candidate); }
                catch (BadImageFormatException) { continue; }
                catch (FileLoadException) { continue; }
                if (identity.Name != "Aki.Game") continue;
                if (found != null) throw new InvalidDataException("AMBIGUOUS_GAME_ASSEMBLY");
                found = new GameplayAssemblyResult { Bytes = image, Name = identity.Name, Version = identity.Version.ToString(), EntryOffset = header };
            }
            finally { if (File.Exists(candidate)) File.Delete(candidate); }
        }
        if (found == null) throw new InvalidDataException("NO_VERIFIED_PLAINTEXT_AKI_GAME_ENTRY");
        return found;
    }
}
