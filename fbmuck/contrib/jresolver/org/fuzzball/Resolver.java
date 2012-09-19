package org.fuzzball;

import java.io.*;
import java.util.*;
import java.lang.*;

class Resolver
{
	private static LinkedList requests = new LinkedList();
	private static final int MAX_HANDLER_THREADS = 8;

	public static void main(String[] argv)
	{
		BufferedReader in = new BufferedReader(new InputStreamReader(System.in));
		OutputStream out = System.out;

		for (int i = 0; i < MAX_HANDLER_THREADS; i++)
		{
			ResolverHandler rh = new ResolverHandler();
			rh.start();
		}

		while(true)
		{
			try
			{
				String line = in.readLine();
				if (line == null)
					break;
				if (line.equals("QUIT"))
					break;

				synchronized (requests)
				{
					requests.addLast(line);
					requests.notify();
				}
			}
			catch (IOException _ex)
			{
				break;
			}
		}
		System.exit(0);
	}

	public static String getRequest()
	{
		String out = null;
		synchronized (requests)
		{
			while (requests.size() < 1)
			{
				try
				{
					requests.wait();
				}
				catch (InterruptedException _ex)
				{
				}
			}
			return (String)requests.removeLast();
		}
	}
}


