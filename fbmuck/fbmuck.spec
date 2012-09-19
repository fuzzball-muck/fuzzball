Summary: 		The FuzzBall TinyMUCK online chat/MUD server
Name: 			fbmuck
Version: 		6.00rc9
Release: 		1
Group: 			Networking/Chat
Distribution:	Linux-Mandrake
Copyright: 		GPL
Url: 			http://sourceforge.net/projects/fbmuck/
Source:			http://belnet.dl.sourceforge.net/sourceforge/fbmuck/%{name}-%{version}.tar.gz
BuildRoot: 		/tmp/%{name}_root


%description 
FuzzBall Muck is a networked multi-user MUD chat server. It is
user-extensible, and supports advanced features such as GUI dialogs,
through close client-server cooperation with Trebuchet or other
clients that support MCP-GUI.

This is the FuzzBall Muck server daemon program.

If you are running this server with a new database.  You should
connect to the server with a MUD client program (or even just telnet),
then log in with the command 'connect #1 potrzebie'.  You should
immediately change your password with the command

@password potrzebie=YOURNEWPASSWORD


%prep
%setup

%build
./configure --prefix=/usr --enable-ssl
make CFLAGS="$RPM_OPT_FLAGS"

%install
make prefix=$RPM_BUILD_ROOT/usr install
install -D -m 755 scripts/fbmuck-redhat-init $RPM_BUILD_ROOT/etc/rc.d/init.d/fbmuck
chmod 755 $RPM_BUILD_ROOT/etc/rc.d/init.d/fbmuck
[ -f $RPM_BUILD_ROOT/etc/fbmucks ] || echo "#MUCKNAME   USERNAME    MUCK_ROOT_PATH           SCRIPTNAME  PORTS" > $RPM_BUILD_ROOT/etc/fbmucks

%clean
rm -rf $RPM_BUILD_ROOT

%post
chkconfig --add fbmuck

%preun
if [ $1 = 0 ]; then
	chkconfig --del fbmuck
	/etc/rc.d/init.d/fbmuck stop
fi

%files
%defattr(-,root,root)
%doc docs/COPYING docs/mufman.html docs/mpihelp.html INSTALLATION README
%config(noreplace) /etc/fbmucks
%config(noreplace) /etc/rc.d/init.d/fbmuck
/usr/bin/fbmuck
/usr/bin/fb-resolver
/usr/bin/fb-topwords
/usr/bin/fb-olddecompress
/usr/bin/fbhelp
/usr/bin/fb-announce
#/usr/sbin/fb-addmuck
/usr/share/fbmuck/help.txt
/usr/share/fbmuck/man.txt
/usr/share/fbmuck/mpihelp.txt
/usr/share/fbmuck/restart-script
#/usr/share/fbmuck/starter_dbs/basedb.tar.gz
#/usr/share/fbmuck/starter_dbs/minimaldb.tar.gz


%changelog

* Sat Nov 09 2002 Schneelocke <schnee@gl00on.net> 6.00rc9-1
- bumped version number to 6.00rc9

* Fri Oct 11 2002 Revar <revar@belfry.com> 6.00rc7-2mdk
- fixed sysv-init scripts issues.

* Sat Sep 07 2002 Schneelocke <schnee@gl00on.net> 6.00rc7-1
- bumped version number to 6.00rc7
- changed Source url to reflect sourceforge changes

* Thu Jun 06 2002 Revar Desmera <revar@belfry.com> 6.00rc6-1
- added make install-inits

* Wed Apr 18 2001 Revar Desmera <revar@belfry.com> 6.00rc1-1
- First try at making the packages

