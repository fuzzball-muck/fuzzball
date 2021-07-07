## General Info and License

STMP Support is provided via the Somnisoft SMTP Client

This client is provided using the Creative Commons 0 Public Domain license.  The original source code is located here:

https://github.com/somnisoft/smtp-client

The authors of Fuzzball MUCK do not make any claim to that code.  The full license is attached below.

This code is located in src/smtp.c and include/smtp.h

You send mail with SMTP_SEND MUF primitive which is documented in 'man'.

## Why?

This enables MUCKs to make a registration process that will email credentials or approval information to the new user.  It could also be used for notifications, monitoring, or other useful things.  It requires an external SMTP server.  Tanabi recommends mailjet (https://www.mailjet.com) as an inexpensive option, though you can also configure it to send via gmail or some other service.

Note that many services have some sort of cap on the number of mails you can send.

If you want to replicate Hope Island MUCK's configuration, you can do the following:

- Present a web form to the user that takes registration information.
- Create a MUF to accept MCP requests to create a user, and then another MUF to enable your wizard staff to moderate the request.
- Send the requests from the web form to the MUCK (we use pyfuzzball as an MCP library - https://github.com/tanabi/pyfuzzball )

If there's interest in doing similar systems, Tanabi will put the Hope Island scripts up, but right now it's in sort of a hacky condition and a litlte hard coded in places so it isn't out of the box useful to anyone else as-is.  But!  That is the intention of this SMTP stuff.

## WARNING

As of this writing, the MUCK is locked up during mail sending.  This usually takes a couple seconds, but can take up to about 10 - 20 seconds especially if you configure something wrong and the MUCK gets stuck waiting for something for the maximum amount of time.

We will seek to improve this in the future if it is a popular feature -- however for the initial use of it, the trade off is worth it.

## Configuration

All configuration is done in MUCK with `@tune` variables.  For security reasons, these variables are only settable by One.  Please note that your SMTP password is stored in clear text in the database and is of course visible to anyone with access to One.  This is sub-optimal and is something we can perhaps fix in the future.

If smtp_server is not set, SMTP_SEND will not work.  It is not set by default.  The following variables can be used:

```
@tune smtp_server=servername
@tune smtp_port=port
@tune smtp_ssl_type=0 for StartSSL (typical), 1 for TLS, 2 for No SSL
@tune smtp_auth_type=0 for CRAM_MD5, 1 for None, 2 for Plain (typical), 3 for login
@tune smtp_user=username
@tune smtp_password=password
@tune smtp_from_name=From User Name
@tune smtp_from_email=from@useremail.com
@tune smtp_no_verify_cert=no
```

## Full License

Creative Commons Legal Code
CC0 1.0 Universal
Official translations of this legal tool are available

CREATIVE COMMONS CORPORATION IS NOT A LAW FIRM AND DOES NOT PROVIDE LEGAL
SERVICES. DISTRIBUTION OF THIS DOCUMENT DOES NOT CREATE AN ATTORNEY-CLIENT
RELATIONSHIP. CREATIVE COMMONS PROVIDES THIS INFORMATION ON AN "AS-IS" BASIS.
CREATIVE COMMONS MAKES NO WARRANTIES REGARDING THE USE OF THIS DOCUMENT OR THE
INFORMATION OR WORKS PROVIDED HEREUNDER, AND DISCLAIMS LIABILITY FOR DAMAGES
RESULTING FROM THE USE OF THIS DOCUMENT OR THE INFORMATION OR WORKS PROVIDED
HEREUNDER.

Statement of Purpose

The laws of most jurisdictions throughout the world automatically confer
exclusive Copyright and Related Rights (defined below) upon the creator and
subsequent owner(s) (each and all, an "owner") of an original work of
authorship and/or a database (each, a "Work").

Certain owners wish to permanently relinquish those rights to a Work for the
purpose of contributing to a commons of creative, cultural and scientific works
("Commons") that the public can reliably and without fear of later claims of
infringement build upon, modify, incorporate in other works, reuse and
redistribute as freely as possible in any form whatsoever and for any purposes,
including without limitation commercial purposes. These owners may contribute
to the Commons to promote the ideal of a free culture and the further
production of creative, cultural and scientific works, or to gain reputation
or greater distribution for their Work in part through the use and efforts of
others.

For these and/or other purposes and motivations, and without any expectation
of additional consideration or compensation, the person associating CC0 with a
Work (the "Affirmer"), to the extent that he or she is an owner of Copyright
and Related Rights in the Work, voluntarily elects to apply CC0 to the Work
and publicly distribute the Work under its terms, with knowledge of his or her
Copyright and Related Rights in the Work and the meaning and intended legal
effect of CC0 on those rights.

1. Copyright and Related Rights.

  A Work made available under CC0 may be protected by copyright and related
  or neighboring rights ("Copyright and Related Rights"). Copyright and
  Related Rights include, but are not limited to, the following:

    the right to reproduce, adapt, distribute, perform, display, communicate,
    and translate a Work;

    moral rights retained by the original author(s) and/or performer(s);

    publicity and privacy rights pertaining to a person's image or likeness
    depicted in a Work;

    rights protecting against unfair competition in regards to a Work, subject
    to the limitations in paragraph 4(a), below;

    rights protecting the extraction, dissemination, use and reuse of data in
    a Work;

    database rights (such as those arising under Directive 96/9/EC of the
    European Parliament and of the Council of 11 March 1996 on the legal
    protection of databases, and under any national implementation thereof,
    including any amended or successor version of such directive); and

    other similar, equivalent or corresponding rights throughout the world
    based on applicable law or treaty, and any national implementations
    thereof.

2. Waiver.

  To the greatest extent permitted by, but not in contravention of, applicable
  law, Affirmer hereby overtly, fully, permanently, irrevocably and
  unconditionally waives, abandons, and surrenders all of Affirmer's Copyright
  and Related Rights and associated claims and causes of action, whether now
  known or unknown (including existing as well as future claims and causes of
  action), in the Work (i) in all territories worldwide, (ii) for the maximum
  duration provided by applicable law or treaty (including future time
  extensions), (iii) in any current or future medium and for any number of
  copies, and (iv) for any purpose whatsoever, including without limitation
  commercial, advertising or promotional purposes (the "Waiver"). Affirmer
  makes the Waiver for the benefit of each member of the public at large and
  to the detriment of Affirmer's heirs and successors, fully intending that
  such Waiver shall not be subject to revocation, rescission, cancellation,
  termination, or any other legal or equitable action to disrupt the quiet
  enjoyment of the Work by the public as contemplated by Affirmer's express
  Statement of Purpose.

3. Public License Fallback.

  Should any part of the Waiver for any reason be judged legally invalid or
  ineffective under applicable law, then the Waiver shall be preserved to the
  maximum extent permitted taking into account Affirmer's express Statement of
  Purpose. In addition, to the extent the Waiver is so judged Affirmer hereby
  grants to each affected person a royalty-free, non transferable, non
  sublicensable, non exclusive, irrevocable and unconditional license to
  exercise Affirmer's Copyright and Related Rights in the Work (i) in all
  territories worldwide, (ii) for the maximum duration provided by applicable
  law or treaty (including future time extensions), (iii) in any current or
  future medium and for any number of copies, and (iv) for any purpose
  whatsoever, including without limitation commercial, advertising or
  promotional purposes (the "License"). The License shall be deemed effective
  as of the date CC0 was applied by Affirmer to the Work. Should any part of
  the License for any reason be judged legally invalid or ineffective under
  applicable law, such partial invalidity or ineffectiveness shall not
  invalidate the remainder of the License, and in such case Affirmer hereby
  affirms that he or she will not (i) exercise any of his or her remaining
  Copyright and Related Rights in the Work or (ii) assert any associated
  claims and causes of action with respect to the Work, in either case
  contrary to Affirmer's express Statement of Purpose.

4. Limitations and Disclaimers.

  No trademark or patent rights held by Affirmer are waived, abandoned,
  surrendered, licensed or otherwise affected by this document.

  Affirmer offers the Work as-is and makes no representations or warranties
  of any kind concerning the Work, express, implied, statutory or otherwise,
  including without limitation warranties of title, merchantability, fitness
  for a particular purpose, non infringement, or the absence of latent or
  other defects, accuracy, or the present or absence of errors, whether or not
  discoverable, all to the greatest extent permissible under applicable law.

  Affirmer disclaims responsibility for clearing rights of other persons that
  may apply to the Work or any use thereof, including without limitation any
  person's Copyright and Related Rights in the Work. Further, Affirmer
  disclaims responsibility for obtaining any necessary consents, permissions
  or other rights required for any use of the Work.

  Affirmer understands and acknowledges that Creative Commons is not a party
  to this document and has no duty or obligation with respect to this CC0 or
  use of the Work.
