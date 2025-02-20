# Changelog
## [1.3.2] - 2025-02-20
### Added
- Feature Home Assistant and Thingboard compatibilty added.
  The topic base is defined in config.h by SYSTEM_NAME/DEVICE_ID
  This means you may have to adjust the prefix path in Home Assistant: 
  Home Assistant uses homeassistant//<object_id>/config for discovery.
  If you choose to set a different topic root here adjust HA
  In HA: update the discovery_prefix parameter to point to your discovery path, e.g., {system_name}/devices/{device_id}/$system/discovery/

- The dashboard and preferences no use token based aut.
  Set the user credentials on the preferences page.
  The logon page has a link for a credential reset procedure.
  Default user=admin and password=admin

  