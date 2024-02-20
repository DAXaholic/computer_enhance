#define PROFILE
#ifdef PROFILE

#ifndef PROFILER_SLOT_COUNT
#define PROFILER_SLOT_COUNT 128
#endif

struct ProfileEntry
{
	u64 Tsc;
	u64 HitCount;
	u64 Elapsed;
	u64 ElapsedChildren;
	int ParentSlotIdx;
	const char *Name;
};

static u64 ProfileStartCounter = 0;
static ProfileEntry Slots[PROFILER_SLOT_COUNT];
static int CurrentSlotIdx = 0;
static u64 ProfileEndCounter = 0;

static void BeginProfile()
{
	ProfileStartCounter = ReadCPUTimer();
}

static void EndProfile()
{
	ProfileEndCounter = ReadCPUTimer();
}

static void BeginProfileBlock(u32 slotIdx, const char *name)
{
	ProfileEntry *entry = Slots + slotIdx;
	entry->Tsc = ReadCPUTimer();
	entry->HitCount++;
	entry->Name = name;
	entry->ParentSlotIdx = CurrentSlotIdx;
	CurrentSlotIdx = slotIdx;
}

static void EndProfileBlock(u32 slotIdx)
{
	ProfileEntry *entry = Slots + slotIdx;

	u64 curTsc = ReadCPUTimer();
	u64 lastTsc = entry->Tsc;
	u64 elapsed = curTsc - lastTsc;

	entry->Tsc = curTsc;
	entry->Elapsed += elapsed;
	CurrentSlotIdx = entry->ParentSlotIdx;

	Slots[CurrentSlotIdx].ElapsedChildren += elapsed;
}

struct ProfileCodeBlock
{
private:
	int m_SlotIdx;

public:
	ProfileCodeBlock(u32 slotIdx, const char *name) : m_SlotIdx(slotIdx)
	{
		BeginProfileBlock(slotIdx, name);
	}

	~ProfileCodeBlock()
	{
		EndProfileBlock(m_SlotIdx);	
	}
};

#define PROFILE_BLOCK(NAME) ProfileCodeBlock profileCodeBlock(__COUNTER__ + 1, #NAME);

static void DumpProfile()
{
	if (ProfileStartCounter == 0)
	{
		fprintf(stderr, "No profile started.\n");
		return;
	}
	if (ProfileEndCounter == 0)
	{
		fprintf(stderr, "Profile still in progress.\n");
		return;
	}
	
	u64 total = ProfileEndCounter - ProfileStartCounter;
	u64 frequency = GuessCPUTimerFreq();

	printf("------------ Profile ------------\n");
	printf("Total: %llu (%.1f ms)\n", total, (double)total * 1000 / frequency);
	for (int i = 1; i <= PROFILER_SLOT_COUNT; i++)
	{
		if (Slots[i].Elapsed == 0) continue;
		
		u64 elapsedIncl = Slots[i].Elapsed;
		u64 elapsedExcl = elapsedIncl - Slots[i].ElapsedChildren;
		printf("%s[%llu]: %llu (%.2f%%)",
			Slots[i].Name,
			Slots[i].HitCount,
			elapsedExcl,
			(double)elapsedExcl * 100 / total);

		if (elapsedIncl != elapsedExcl)
		{
			printf(" excl. / %llu (%.2f%%) incl.",
				elapsedIncl,
				(double)elapsedIncl * 100 / total);
		}
		printf("\n");
	}
}

#endif