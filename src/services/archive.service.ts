import { API_BASE_URL } from './api.service';

interface ArchiveResult {
  success: boolean;
  archivedCount: number;
  remainingCount: number;
  archiveFile: string;
  message?: string;
}

interface ClearResult {
  success: boolean;
  clearedCount: number;
  backupFile: string;
  message?: string;
}

/**
 * Archive orders older than specified months
 * @param monthsOld - Archive orders older than this many months (default: 1)
 */
export const archiveOldOrders = async (monthsOld: number = 1): Promise<ArchiveResult> => {
  try {
    const response = await fetch(`${API_BASE_URL}/orders/archive`, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
      },
      body: JSON.stringify({ monthsOld }),
    });
    
    if (!response.ok) {
      throw new Error('Failed to archive orders');
    }
    
    return await response.json();
  } catch (error) {
    console.error('Archive error:', error);
    throw error;
  }
};

/**
 * Clear ALL orders (creates backup first)
 * WARNING: This deletes all orders from the active database
 */
export const clearAllOrders = async (): Promise<ClearResult> => {
  try {
    const response = await fetch(`${API_BASE_URL}/orders/clear`, {
      method: 'DELETE',
    });
    
    if (!response.ok) {
      throw new Error('Failed to clear orders');
    }
    
    return await response.json();
  } catch (error) {
    console.error('Clear error:', error);
    throw error;
  }
};
