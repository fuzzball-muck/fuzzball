(*
 * trigger-dump.muf
 *
 * This is the MUF companion to trigger-dump.py that waits for an MCP
 * event, triggers a dump, then returns an MCP-event when the dump is
 * complete.
 *
 * It is not part of the regular distribution because it requires some
 * configuration.  PLEASE change the password in this file to something
 * that is unique to your MUCK, otherwise anyone with access to the
 * fuzzball source [i.e. everyone] can just dump your database constantly.
 *
 * This program should be set 'A' for autorun.
 *
 * If you are also using mcp-extern.muf, you can easily merge this code
 * into that and have 1 listener MUF instead of 2 :]
 *
 * - tanabi
 *)
 
$def AUTHTOKEN   "change-me-please"
$def ALLOWED_HOSTS "::1" "localhost" "127.0.0.1"
 
: check_auth[ int:dscr dict:args -- int:is_ok ]
  { ALLOWED_HOSTS }list dscr @ descrhost 
  array_findval not if 0 exit then
  
  args @ "auth" []
  
  dup array? not if
    pop 0 exit
  then
  
  0 []
  AUTHTOKEN strcmp not
;
 
(* Keep track of the descriptor that did the dumping so we can notify them *)
lvar dumping_descr
 
: do_dump[ int:dscr dict:args -- ]
  dscr @ dumping_descr !
  DUMP not if
    dscr @ [] "org-fuzzball-dump" "dump" {
      "failed" "Already Dumping"
    }dict mcp_send
    -1 dumping_descr !
  then
;
 
: report_dump ( -- )
  dumping_descr @ 0 < if
    (* This was probably an auto-dump *)
    exit
  then
  
  dumping_descr @ "org-fuzzball-dump" "dump" {
    "success" "Dump Finished"
  }dict mcp_send
  
  -1 dumping_descr !
;
 
: main ( s -- )
  pop
  "org-fuzzball-dump" 1.0 1.0 MCP_REGISTER_EVENT
  
  -1 dumping_descr !
  
  begin
    {
      "MCP.org-fuzzball-dump-dump" 'do_dump
      "DUMP" 'report_dump
    }dict

    dup array_keys array_make
    event_waitfor var! event var! ctx

    event @ [] dup if
      (* For DUMP, this will be an integer 1 *)
      ctx @ int? if
        execute (* Run report dump *)
      else
        ctx @ "descr" []
        ctx @ "args" []
        over over check_auth if
          rot execute
        else
          pop pop pop
        then
      then
    else
      pop
    then
  repeat
;
