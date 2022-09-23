using System.IO;
using System.Reflection;
using System;
using System.Diagnostics;

namespace UnitTest
{
	internal partial class Tests : TestsBase
	{
		private bool BackupInfoMemory()
		{
			// GetFeatures is required to identify the emulator
			if (feats_.Count == 0
				&& !GetFeatures())
				return false;

			Utility.WriteLine("BACKUP INFO MEMORY");
			// Compose backup folder and file name
			String? folder = Assembly.GetExecutingAssembly().Location;
			if (!String.IsNullOrEmpty(folder))
				folder = Path.GetDirectoryName(folder);
			if (folder == null)
				throw new Exception("General inconsistency on application path!");
			folder = Path.Combine(folder, "backup");
			if (folder == null)
				throw new Exception("General inconsistency on application path!");
			info_file_ = Path.Combine(folder, String.Format("{0}_{1}.bin"
				, chip_.name
				, DateTime.Now.ToString("yyddMM-HHmmss")));
			// Select the INFO memory
			MemBlock? memBlock = SelectInfoMemory();
			if (memBlock == null)
				return false;
			Utility.WriteLine("  Using INFO at 0x{0:X4} ({1} bytes)", memBlock.mem_start_, memBlock.mem_size_);
			info_backup_ = new byte[memBlock.mem_size_];
			uint pos = 0;
			while (pos < memBlock.mem_size_)
			{
				UInt32 blk = memBlock.mem_size_ - pos;
				if (blk > 512)
					blk = 512;
				if (!ReadMemCompatible(memBlock.mem_start_ + pos, blk, new Span<byte>(info_backup_, (int)pos, (int)blk)))
					return false;
				// Next iteration
				pos += blk;
			}
			// Check if INFO memory contains usable data
			bool dirty = false;
			foreach (byte b in info_backup_)
			{
				if (b != 0xff)
				{
					dirty = true;
					break;
				}
			}
			// Backup dirty info memories
			if (dirty)
			{
				Directory.CreateDirectory(folder);
				System.IO.File.WriteAllBytes(info_file_, info_backup_);
				Utility.WriteLine("  OK! Wrote '{0}' file", info_file_);
			}
			else
			{
				Utility.WriteLine("  WARNING! INFO memory is erased! Skipping...");
			}
			return true;
		}

		// Checks if INFO memory was changed
		private bool VerifyInfoMemory()
		{
			Utility.WriteLine("VERIFY INFO MEMORY");
			if (info_backup_ == null)
			{
				Utility.WriteLine("  ERROR! No INFO memory was read!");
				return false;
			}

			// Select the INFO memory
			MemBlock? memBlock = SelectInfoMemory();
			if (memBlock == null)
				return false;
			Utility.WriteLine("  Using INFO at 0x{0:X4} ({1} bytes)", memBlock.mem_start_, memBlock.mem_size_);
			byte[] buf_in = new byte[memBlock.mem_size_];
			uint pos = 0;
			while (pos < memBlock.mem_size_)
			{
				UInt32 blk = memBlock.mem_size_ - pos;
				if (blk > 512)
					blk = 512;
				if (!ReadMemCompatible(memBlock.mem_start_ + pos, blk, new Span<byte>(buf_in, (int)pos, (int)blk)))
					return false;
				// Next iteration
				pos += blk;
			}
			// Check if INFO memory contains usable data
			for (int i = 0; i < memBlock.mem_size_; ++i)
			{
				if (info_backup_[i] != buf_in[i])
				{
					Utility.WriteLine("  ERROR! INFO memory has changed! @0x{0:X4} = 0x{1:X2}, was 0x{2:X2}"
						, memBlock.mem_start_ + i, buf_in[i], info_backup_[i]);
					return false;
				}
			}
			Utility.WriteLine("  OK!");
			return true;
		}

		private bool RestoreInfoMemory()
		{
			Utility.WriteLine("BACKUP INFO MEMORY");
			if (info_backup_ == null)
			{
				Utility.WriteLine("  ERROR! No INFO memory was read!");
				return false;
			}

			// Select the INFO memory
			MemBlock? memBlock = SelectInfoMemory();
			if (memBlock == null)
				return false;
			Utility.WriteLine("  Using INFO at 0x{0:X4} ({1} bytes)", memBlock.mem_start_, memBlock.mem_size_);
			uint pos = 0;
			while (pos < memBlock.mem_size_)
			{
				UInt32 blk = memBlock.mem_size_ - pos;
				if (blk > 512)
					blk = 512;
				if (!WriteMemCompatible(memBlock.mem_start_ + pos, new Span<byte>(info_backup_, (int)pos, (int)blk)))
					return false;
				// Next iteration
				pos += blk;
			}
			Utility.WriteLine("  OK! Wrote '{0}' bytes", memBlock.mem_size_);
			return true;
		}

		private bool EraseFlash()
		{
			// GetFeatures is required to identify the emulator
			if (feats_.Count == 0
				&& !GetFeatures())
				return false;

			Utility.WriteLine("ERASING FLASH");
			MemBlock? memBlock = SelectFlashMemory();
			if (memBlock == null)
			{
				Utility.WriteLine("  WARNING! MCU does not contain flash memory! Skipping...");
				return true;
			}
			if (comm_.GetPlatform() == Platform.gdb_agent)
			{
				Utility.WriteLine("  WARNING! Emulator does not support an erase command! Skipping...");
				return true;
			}

			String cmd;
			if (comm_.GetPlatform() == Platform.glossy_msp)
				cmd = "erase all";
			else
				cmd = "erase";
			// Sends request
			SendMonitor(cmd);
			// Get response string
			String msg;
			if (!DecodeHexToString(out msg))
				return false;
			if (String.IsNullOrEmpty(msg))
			{
				Utility.WriteLine("  Command 'mon {0}' was not recognized", cmd);
				return false;
			}
			// Report message from emulator
			Utility.WriteLine("  " + msg.TrimEnd());
			return true;
		}

		private bool VerifyFlashErased()
		{
			Utility.WriteLine("VERIFY IF FLASH IS ERASED");
			MemBlock? memBlock = SelectFlashMemory();
			if (memBlock == null)
			{
				Utility.WriteLine("  WARNING! MCU does not contain flash memory! Skipping...");
				return true;
			}
			Utility.WriteLine("  Using FLASH/ROM at 0x{0:X4} ({1} bytes)", memBlock.mem_start_, memBlock.mem_size_);
			// Scan memory
			uint addr = memBlock.mem_start_;
			uint maxmem = memBlock.mem_start_ + memBlock.mem_size_;
			byte[] buf_out = new byte[512];
			bool valid = true;
			while (addr < maxmem)
			{
				uint size = maxmem - addr;
				if (size > 512)
					size = 512;
				// Read a flash page
				if (!ReadMemCompatible(addr, size, new Span<byte>(buf_out)))
					return false;
				for (uint i = 0; i < size; i++)
				{
					// Erased flash should have only 0xFF values
					if (buf_out[i] != 0xff)
					{
						valid = false;
						break;
					}
				}
				// Compare error
				if (!valid)
					break;
				addr += size;
			}
			// Compare error
			if (!valid)
			{
				Utility.WriteLine("  ERROR! Memory is not erased and still contains data!");
				return false;
			}
			Utility.WriteLine("  Verification PASSED!");
			return true;
		}

		private bool TestFlashWrite()
		{
			Utility.WriteLine("TEST FLASH WRITE");
			MemBlock? memBlock = SelectFlashMemory();
			if (memBlock == null)
				return false;
			Utility.WriteLine("  Using FLASH at 0x{0:X4} ({1} bytes)", memBlock.mem_start_, memBlock.mem_size_);
			byte[] buf_out = new byte[memBlock.mem_size_];
			Random rnd = new Random(1234);
			rnd.NextBytes(buf_out);
			Stopwatch sw = Stopwatch.StartNew();
			UInt32 pos = 0;
			while (pos < memBlock.mem_size_)
			{
				UInt32 blk = memBlock.mem_size_ - pos;
				if (blk > 512)
					blk = 512;
				if (!WriteMemCompatible(memBlock.mem_start_ + pos, new Span<byte>(buf_out, (int)pos, (int)blk)))
					return false;
				// Next iteration
				pos += blk;
			}
			long elapsed = sw.ElapsedMilliseconds;
			if (elapsed == 0)
				elapsed = 1;    // avoid division by 0
			Utility.WriteLine("  Write Performance: {0:0.00} kB/s", (double)(1000 * memBlock.mem_size_) / (elapsed * 1024));

			Utility.WriteLine("  VERIFICATION...");
			pos = 0;
			while (pos < memBlock.mem_size_)
			{
				UInt32 blk = memBlock.mem_size_ - pos;
				if (blk > 512)
					blk = 512;
				if (!VerifyMemCompatible(memBlock.mem_start_ + pos, new Span<byte>(buf_out, (int)pos, (int)blk)))
					return false;
				// Next iteration
				pos += blk;
			}
			Utility.WriteLine("  Verification PASSED!");
			return true;
		}

		// Verify and restore info memory
		private bool CheckSafeInfo(bool del_file)
		{
			// 401
			if (!VerifyInfoMemory())
			{
				// 402
				if (!RestoreInfoMemory())
					Utility.WriteLine("ATTENTION! Could not restore Info memory. Consider to restore the last backup file.");
				return false;
			}
			if (del_file && info_file_ != null)
			{
				System.IO.File.Delete(info_file_);
				Utility.WriteLine("  SUCCESS! removed backup file '{0}'", Path.GetFileName(info_file_));
			}
			return true;
		}

		// Test 2 erases flash memory
		private bool Test2()
		{
			// 100
			if (!GetFeatures())
				return false;
			// 110
			if (!GetReplyMode())
				return false;
			// 120
			if (!StartNoAckMode())
				return false;
			// 220
			if (!GetRegisterValues())
				return false;
			// 400
			if (!BackupInfoMemory())
				return false;
			// 220
			if (!CompareRegisterValues())
				return false;
			// 410
			if (!EraseFlash())
				return false;
			// Be sure Info Mem is OK
			if (!CheckSafeInfo(false))
				return false;
			bool res =
				// 221
				CompareRegisterValues()
				// 420
				&& VerifyFlashErased()
				// 221
				&& CompareRegisterValues()
				// 430
				&& TestFlashWrite()
				// 221
				&& CompareRegisterValues()
				// 410
				&& EraseFlash()
				// 221
				&& CompareRegisterValues()
				// 420
				&& VerifyFlashErased()
				// 221
				&& CompareRegisterValues()
				;
			// Restore any damage to the INFO memory
			res = CheckSafeInfo(res);
			// 9999
			if (!Detach())
				res = false;
			return res;
		}
	}
}
