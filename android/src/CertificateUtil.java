package org.xcsoar;

import android.content.Context;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.security.KeyStore;
import java.security.cert.CertificateEncodingException;
import java.security.cert.X509Certificate;
import javax.net.ssl.TrustManagerFactory;
import javax.net.ssl.X509TrustManager;

/**
 * Utility class for extracting Android system CA certificates
 * and converting them to PEM format for use with libcurl.
 */
public class CertificateUtil {
  /**
   * Extract all system CA certificates and write them to a PEM file.
   * 
   * @param context Android context for accessing cache directory
   * @return Absolute path to the PEM file, or null on failure
   */
  public static String extractSystemCertificates(Context context) {
    try {
      // Get system trust manager
      TrustManagerFactory factory = TrustManagerFactory.getInstance(
        TrustManagerFactory.getDefaultAlgorithm());
      factory.init((KeyStore) null); // Use system keystore
      
      javax.net.ssl.TrustManager[] trustManagers = factory.getTrustManagers();
      if (trustManagers == null || trustManagers.length == 0) {
        return null;
      }
      
      if (!(trustManagers[0] instanceof X509TrustManager)) {
        return null;
      }
      
      X509TrustManager trustManager = (X509TrustManager) trustManagers[0];
      X509Certificate[] certificates = trustManager.getAcceptedIssuers();
      
      if (certificates == null || certificates.length == 0) {
        return null;
      }
      
      // Write certificates to PEM file in cache directory
      File pemFile = new File(context.getCacheDir(), "cacerts.pem");
      try (FileOutputStream fos = new FileOutputStream(pemFile)) {
        for (X509Certificate cert : certificates) {
          // Write PEM header
          fos.write("-----BEGIN CERTIFICATE-----\n".getBytes("UTF-8"));
          
          // Write Base64-encoded certificate (64 chars per line)
          byte[] encoded = cert.getEncoded();
          String base64 = android.util.Base64.encodeToString(
            encoded, android.util.Base64.NO_WRAP);
          
          // Split into 64-character lines
          for (int i = 0; i < base64.length(); i += 64) {
            int end = Math.min(i + 64, base64.length());
            fos.write(base64.substring(i, end).getBytes("UTF-8"));
            fos.write('\n');
          }
          
          // Write PEM footer
          fos.write("-----END CERTIFICATE-----\n".getBytes("UTF-8"));
        }
      }
      
      return pemFile.getAbsolutePath();
    } catch (Exception e) {
      android.util.Log.e("XCSoar", "Failed to extract system certificates", e);
      return null;
    }
  }
}

