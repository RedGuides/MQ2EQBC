---
tags:
  - datatype
---
# `EQBC`

<!--dt-desc-start-->
Members of this datatype relate to MQ2EQBC settings and generic information and can be accessed via the ${EQBC} TLO.
<!--dt-desc-end-->

## Members
<!--dt-members-start-->
### {{ renderMember(type='bool', name='Connected') }}

:   Client connection status

### {{ renderMember(type='string', name='Server') }}

:   Returns hostname/ip of the connected server

### {{ renderMember(type='string', name='Port') }}

:   Returns port of the connected server

### {{ renderMember(type='string', name='ToonName') }}

:   Character name as seen by EQBC (may reflect YouPlayer)

### {{ renderMember(type='bool', name='Setting', params='option') }}

:   On/Off status of specified option (/bccmd set for list)

### {{ renderMember(type='string', name='Names') }}

:   List of connected characters

### {{ renderMember(type='bool', name='GotNames') }}

:   Indicates whether your client has received the name list from the server

<!--dt-members-end-->

## Examples
<!--dt-examples-start-->
- `/echo ${EQBC.Names}` returns "gSe7eN eqmule plure sym1 redbot Warl0ck45"
- `/echo ${EQBC.Port}` returns "2112"
<!--dt-examples-end-->

<!--dt-linkrefs-start-->
[bool]: ../macroquest/reference/data-types/datatype-bool.md
[string]: ../macroquest/reference/data-types/datatype-string.md
<!--dt-linkrefs-end-->
