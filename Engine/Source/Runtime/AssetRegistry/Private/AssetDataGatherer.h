// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * Minimal amount of information needed about a discovered asset file
 */
struct FDiscoveredPackageFile
{
	explicit FDiscoveredPackageFile(FString InPackageFilename)
		: PackageFilename(MoveTemp(InPackageFilename))
		, PackageTimestamp(IFileManager::Get().GetTimeStamp(*PackageFilename))
	{
	}

	FDiscoveredPackageFile(FString InPackageFilename, FDateTime InPackageTimestamp)
		: PackageFilename(MoveTemp(InPackageFilename))
		, PackageTimestamp(MoveTemp(InPackageTimestamp))
	{
	}

	FDiscoveredPackageFile(const FDiscoveredPackageFile& Other)
		: PackageFilename(Other.PackageFilename)
		, PackageTimestamp(Other.PackageTimestamp)
	{
	}

	FDiscoveredPackageFile(FDiscoveredPackageFile&& Other)
		: PackageFilename(MoveTemp(Other.PackageFilename))
		, PackageTimestamp(MoveTemp(Other.PackageTimestamp))
	{
	}

	FDiscoveredPackageFile& operator=(const FDiscoveredPackageFile& Other)
	{
		if (this != &Other)
		{
			PackageFilename = Other.PackageFilename;
			PackageTimestamp = Other.PackageTimestamp;
		}
		return *this;
	}

	FDiscoveredPackageFile& operator=(FDiscoveredPackageFile&& Other)
	{
		if (this != &Other)
		{
			PackageFilename = MoveTemp(Other.PackageFilename);
			PackageTimestamp = MoveTemp(Other.PackageTimestamp);
		}
		return *this;
	}

	/** The name of the package file on disk */
	FString PackageFilename;

	/** The modification timestamp of the package file (when it was discovered) */
	FDateTime PackageTimestamp;
};


/**
 * Async task for discovering files that FAssetDataGatherer should search
 */
class FAssetDataDiscovery : public FRunnable
{
public:
	/** Constructor */
	FAssetDataDiscovery(const TArray<FString>& InPaths, bool bInIsSynchronous);
	virtual ~FAssetDataDiscovery();

	// FRunnable implementation
	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Stop() override;
	virtual void Exit() override;

	/** Signals to end the thread and waits for it to close before returning */
	void EnsureCompletion();

	/** Gets search results from the file discovery */
	bool GetAndTrimSearchResults(TArray<FString>& OutDiscoveredPaths, TArray<FDiscoveredPackageFile>& OutDiscoveredFiles, int32& OutNumPathsToSearch);

	/** Adds a root path to the search queue. Only works when searching asynchronously */
	void AddPathToSearch(const FString& Path);

	/** If assets are currently being asynchronously scanned in the specified path, this will cause them to be scanned before other assets. */
	void PrioritizeSearchPath(const FString& PathToPrioritize);

private:
	/** Sort the paths so that items belonging to the current priority path is processed first */
	void SortPathsByPriority(const int32 MaxNumToSort);

	/** A critical section to protect data transfer to other threads */
	FCriticalSection WorkerThreadCriticalSection;

	/** The current path to prioritize. Files and folders under this location will be bumped to the top of the processing list as they are discovered */
	FString FilenamePathToPrioritize;

	/** True if this gather request is synchronous (i.e, IsRunningCommandlet()) */
	bool bIsSynchronous;

	/** True if in the process of discovering files in PathsToSearch */
	bool bIsDiscoveringFiles;

	/** The directories in which to discover assets and paths */
	// IMPORTANT: This variable may be modified by from a different thread via a call to AddPathToSearch(), so access 
	//            to this array should be handled very carefully.
	TArray<FString> DirectoriesToSearch;

	/** The paths found during the search. It is not threadsafe to directly access this array */
	TArray<FString> DiscoveredPaths;

	/** List of priority files that need to be processed by the gatherer. It is not threadsafe to directly access this array */
	TArray<FDiscoveredPackageFile> PriorityDiscoveredFiles;

	/** List of non-priority files that need to be processed by the gatherer. It is not threadsafe to directly access this array */
	TArray<FDiscoveredPackageFile> NonPriorityDiscoveredFiles;

	/** > 0 if we've been asked to abort work in progress at the next opportunity */
	FThreadSafeCounter StopTaskCounter;

	/** Thread to run the discovery FRunnable on */
	FRunnableThread* Thread;
};


/**
 * Async task for gathering asset data from from the file list in FAssetRegistry
 */
class FAssetDataGatherer : public FRunnable
{
public:
	/** Constructor */
	FAssetDataGatherer(const TArray<FString>& Paths, bool bInIsSynchronous, bool bInLoadAndSaveCache = false);
	virtual ~FAssetDataGatherer();

	// FRunnable implementation
	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Stop() override;
	virtual void Exit() override;

	/** Signals to end the thread and waits for it to close before returning */
	void EnsureCompletion();

	/** Gets search results from the data gatherer */
	bool GetAndTrimSearchResults(TArray<IGatheredAssetData*>& OutAssetResults, TArray<FString>& OutPathResults, TArray<FPackageDependencyData>& OutDependencyResults, TArray<double>& OutSearchTimes, int32& OutNumFilesToSearch, int32& OutNumPathsToSearch, bool& OutIsDiscoveringFiles);

	/** Adds a root path to the search queue. Only works when searching asynchronously */
	void AddPathToSearch(const FString& Path);

	/** Adds specific files to the search queue. Only works when searching asynchronously */
	void AddFilesToSearch(const TArray<FString>& Files);

	/** If assets are currently being asynchronously scanned in the specified path, this will cause them to be scanned before other assets. */
	void PrioritizeSearchPath(const FString& PathToPrioritize);

private:
	/** Sort the paths so that items belonging to the current priority path is processed first */
	void SortPathsByPriority(const int32 MaxNumToSort);

	/**
	 * Reads FAssetData information out of a file
	 *
	 * @param AssetFilename the name of the file to read
	 * @param AssetDataList the FBackgroundAssetData for every asset found in the file
	 * @param DependencyData the FPackageDependencyData for every asset found in the file
	 *
	 * @return true if the file was successfully read
	 */
	bool ReadAssetFile(const FString& AssetFilename, TArray<FBackgroundAssetData*>& AssetDataList, FPackageDependencyData& DependencyData) const;

	/** Serializes the timestamped cache of discovered assets. Used for quick loading of data for assets that have not changed on disk */
	void SerializeCache(FArchive& Ar);

	/** Creates asset data reconstructing all the required info from linker tables */
	FBackgroundAssetData* CreateAssetDataFromLinkerTables(const FString& AssetFilename, uint32 InPackageFlags, const FObjectExport& AssetExport, const TArray<FObjectImport>& ImportMap, const TArray<FObjectExport>& ExportMap) const;

	/** Creates asset data reconstructing all the required data from cooked package info */
	FBackgroundAssetData* CreateAssetDataFromCookedPackage(const FString& AssetFilename, uint32 InPackageFlags, FPackageReader& PackageReader) const;

private:
	/** A critical section to protect data transfer to the main thread */
	FCriticalSection WorkerThreadCriticalSection;

	/** The current path to prioritize. Files and folders under this location will be bumped to the top of the processing list as they are discovered */
	FString FilenamePathToPrioritize;

	/** List of files that need to be processed by the search. It is not threadsafe to directly access this array */
	TArray<FDiscoveredPackageFile> FilesToSearch;

	/** > 0 if we've been asked to abort work in progress at the next opportunity */
	FThreadSafeCounter StopTaskCounter;

	/** True if this gather request is synchronous (i.e, IsRunningCommandlet()) */
	bool bIsSynchronous;

	/** True if in the process of discovering files in PathsToSearch */
	bool bIsDiscoveringFiles;

	/** The current search start time */
	double SearchStartTime;

	/** The asset data gathered from the searched files */
	TArray<IGatheredAssetData*> AssetResults;

	/** Dependency data for scanned packages */
	TArray<FPackageDependencyData> DependencyResults;

	/** All the search times since the last main thread tick */
	TArray<double> SearchTimes;

	/** The paths found during the search */
	TArray<FString> DiscoveredPaths;

	/** True if dependency data should be gathered */
	bool bGatherDependsData;

	/** The cached value of the NumPathsToSearch returned by FAssetDataDiscovery the last time we synchronized with it */
	int32 NumPathsToSearchAtLastSyncPoint;

	/** Background package file discovery (when running async) */
	TSharedPtr<FAssetDataDiscovery> BackgroundPackageFileDiscovery;

	///////////////////////////////////////////////////////////////
	// Asset discovery caching
	///////////////////////////////////////////////////////////////

	/** True if this gather request should both load and save the asset cache. Only one gatherer should do this at a time! */
	bool bLoadAndSaveCache;

	/** True if we have finished discovering our first wave of files */
	bool bFinishedInitialDiscovery;

	/** The name of the file that contains the timestamped cache of discovered assets */
	FString CacheFilename;

	/** When loading a registry from disk, we can allocate all the FAssetData objects in one chunk, to save on 10s of thousands of heap allocations */
	FDiskCachedAssetData* DiskCachedAssetDataBuffer;

	/** An array of all cached data that was newly discovered this run. This array is just used to make sure they are all deleted at shutdown */
	TArray<FDiskCachedAssetData*> NewCachedAssetData;

	/** Map of PackageName to cached discovered assets that were loaded from disk */
	TMap<FName, FDiskCachedAssetData*> DiskCachedAssetDataMap;

	/** Map of PackageName to cached discovered assets that will be written to disk at shutdown */
	TMap<FName, FDiskCachedAssetData*> NewCachedAssetDataMap;

	/** Thread to run the cleanup FRunnable on */
	FRunnableThread* Thread;
};
