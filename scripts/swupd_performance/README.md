
# Swupd Performance scripts #
These scripts have been created for gathering statistics on swupd update performance.

They have been broken down to two parts:
- Setup script (to setup a clear machine's autoupdate feature to output to files)
- Data processing script (to process files from previous step to create entry in
  csv file)

## Swupd Performance logging script ##
Run below command to create a folder `/var/log/swupd/staging` if it does not exist. The `data_processing_script.py` will default to using this folder as an input folder while processing

### Perform setup ###
```
sudo ./swupd_logging_setup.bash
```

### Restore system to clean state without config, staging directory & their contents ###
```
sudo ./swupd_logging_setup.bash restore
```

## Swupd Performance data processing script ##

Run

```
sudo ./data_processing_script.py -f <ABS_PATH_TO_STAGING_FOLDER>
```

**For e.g:**
In your have repo cloned in $HOME, run on samples folder which has sample files from swupd update. It creates an output file performance_sheet.csv by default 
```
sudo ./data_processing_script.py -f $HOME/swupd-client/scripts/swupd_performance/samples
```

**OR**

```
sudo ./data_processing_script.py
```

to use `/var/log/swupd/staging` as default input folder 
