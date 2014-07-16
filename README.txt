;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; References Integrity
;;
;;
;; Original author: markus_petrux (http://drupal.org/user/39593)
;; Drupal 7 author: claudiu.cristea (http://drupal.org/user/56348)
;; Entity reference extension: Floydm (http://drupal.org/user/502450)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

OVERVIEW
========

This module provides a method to enforce referential integrity rules for
References (http://drupal.org/project/references) fields (user and node
reference) or Entity References (https://www.drupal.org/project/entityreference).
In other words, it allows you to decide what to do with reference
values when a referenced entity (node, user, or entity) is deleted.

The problem:

Reference fields may contain values that point to entities, nodes, or users that
have been removed. References module does nothing when a node or user is removed
with the values of fields that reference them. So after some time, your node and
user reference fields may contain records that point to nowhere. Orphans.

- Extension for reference fields:

  Once the module has been installed, the user and node reference field
  settings form will provide a new option "Referential integrity behavior".

  Available options are:

  - None (default).
  - Delete also field items when referenced entities are deleted.
  - Delete also referencing entities when referenced entities are deleted.

- Monitoring orphan records:

  You can also review all your reference fields to monitor if they have orphan
  records from Administer -> Content -> Orphan references.

  This report displays the total number of records in the instances used for
  each reference field, and also the number of orphan records found, if any.

  For the instances where orphan records have been found, you can then list
  these records to review them manually, or to set their values to null. You
  should make backups of your database before processing orphans should you need
  to revert any changes made from this panel.

- Exporting settings:

  Drupal 7 does not allow modules other that the modules that define a field add
  settings to a field (see https://www.drupal.org/node/1308860), so all field
  settings are stored in the variables table and can be exported with Strongarm.

REQUIREMENTS
============

  References or Entity References to make this module useful.

INSTALLATION
============

- Be sure to install all dependent modules.

- Copy all contents of this package to your modules directory preserving
  subdirectory structure.

- Enable the module.

- Review the settings of your reference fields. You will
  find a new option labelled "Referential integrity behavior". It is
  disabled by default.

- View the orphaned references report at /admin/content/orphan-references

CREDITS
=======

- Original versions of the icons can be found free from here:

  http://www.famfamfam.com/
