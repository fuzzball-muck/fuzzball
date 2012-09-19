package org.fuzzball;

import java.io.*;
import java.net.*;
import java.util.*;
import java.lang.*;

class ResolverHandler extends Thread
{
	private static Hashtable addrCache = new Hashtable(8192);

	public ResolverHandler()
	{
	}

	public void run()
	{
		while (true)
		{
			try
			{
				String request = Resolver.getRequest();
				StringTokenizer st = new StringTokenizer(request, "(", true);
				String address = st.nextToken();
				st.nextToken();

				String remotePortStr = st.nextToken(")");
				st.nextToken();
				String localPortStr  = st.nextToken();

				int remotePort = Integer.parseInt(remotePortStr);
				int localPort  = Integer.parseInt(localPortStr);

				String hostname = getHostLookup(address);
				String username = getUserName(address, remotePort, localPort);

				System.out.println(address + "(" + remotePort + "):" + hostname + "(" + username + ")");
				System.out.flush();
			}
			catch (Exception _ex)
			{
			}
		}
	}

	String getHostLookup(String addr)
	{
		String host = null;

		synchronized (addrCache)
		{
			host = (String)addrCache.get(addr);
		}

		if (host == null)
		{
			try
			{
				InetAddress ia = InetAddress.getAllByName(addr)[0];
				host = ia.getHostName();
				synchronized (addrCache)
				{
					addrCache.put(addr, host);
				}
			}
			catch(UnknownHostException _ex)
			{
				host = addr;
			}
		}
		return host;
	}

	String getUserName(String addr, int remotePort, int localPort)
	{
		try
		{
			Socket sock = new Socket(addr, 113);
			sock.setSoTimeout(30000);
			BufferedReader authIn = new BufferedReader(new InputStreamReader(sock.getInputStream()));
			OutputStream authOut = sock.getOutputStream();

			authOut.write(("" + remotePort + "," + localPort + "\n").getBytes());
			String response = authIn.readLine();
			sock.close();

			StringTokenizer st = new StringTokenizer(response, ":");
			st.nextToken();
			String errcode = st.nextToken().trim();
			st.nextToken();
			response = st.nextToken().trim();

			if (errcode == null || !errcode.equals("USERID"))
				return "" + remotePort;
			if (response == null)
				return "" + remotePort;

			return response;
		}
		catch (Exception _ex)
		{
			return "" + remotePort;
		}
	}
}

