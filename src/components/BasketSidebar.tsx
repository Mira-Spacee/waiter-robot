import { useState } from 'react';
import { useOrders } from '@/contexts/OrderContext';
import { Sheet, SheetContent, SheetHeader, SheetTitle, SheetTrigger } from '@/components/ui/sheet';
import { Button } from '@/components/ui/button';
import { ShoppingCart, Minus, Plus, Trash2 } from 'lucide-react';
import { Badge } from '@/components/ui/badge';
import { Select, SelectContent, SelectItem, SelectTrigger, SelectValue } from '@/components/ui/select';
import { toast } from 'sonner';

export const BasketSidebar = () => {
  const { basket, addToBasket, removeFromBasket, placeOrder, clearBasket } = useOrders();
  const [selectedTable, setSelectedTable] = useState<string>('');
  const [open, setOpen] = useState(false);
  const [isPlacingOrder, setIsPlacingOrder] = useState(false);

  const total = basket.reduce((sum, item) => sum + (item.price * item.quantity), 0);
  const itemCount = basket.reduce((sum, item) => sum + item.quantity, 0);

  const handleConfirm = async () => {
    if (!selectedTable) {
      toast.error('Please select a table');
      return;
    }
    
    setIsPlacingOrder(true);
    try {
      await placeOrder(parseInt(selectedTable));
      setSelectedTable('');
      setOpen(false);
      toast.success('Order placed successfully!');
    } catch (error) {
      toast.error('Failed to place order. Please try again.');
    } finally {
      setIsPlacingOrder(false);
    }
  };

  return (
    <Sheet open={open} onOpenChange={setOpen}>
      <SheetTrigger asChild>
        <Button size="icon" className="fixed bottom-6 right-6 h-14 w-14 rounded-full shadow-lg">
          <ShoppingCart className="h-6 w-6" />
          {itemCount > 0 && (
            <Badge className="absolute -top-2 -right-2 h-6 w-6 rounded-full p-0 flex items-center justify-center">
              {itemCount}
            </Badge>
          )}
        </Button>
      </SheetTrigger>
      <SheetContent className="w-full sm:max-w-md">
        <SheetHeader>
          <SheetTitle>Your Basket</SheetTitle>
        </SheetHeader>
        
        <div className="flex flex-col h-full mt-6">
          {basket.length === 0 ? (
            <div className="flex-1 flex items-center justify-center text-muted-foreground">
              Your basket is empty
            </div>
          ) : (
            <>
              <div className="flex-1 overflow-auto space-y-4">
                {basket.map(item => (
                  <div key={item.id} className="flex gap-3 p-3 bg-muted rounded-lg">
                    <img 
                      src={item.image} 
                      alt={item.name}
                      className="w-20 h-20 object-cover rounded"
                    />
                    <div className="flex-1">
                      <h4 className="font-semibold">{item.name}</h4>
                      <p className="text-sm text-muted-foreground">${item.price.toFixed(2)}</p>
                      <div className="flex items-center gap-2 mt-2">
                        <Button
                          size="icon"
                          variant="outline"
                          className="h-7 w-7"
                          onClick={() => removeFromBasket(item.id)}
                        >
                          {item.quantity === 1 ? <Trash2 className="h-3 w-3" /> : <Minus className="h-3 w-3" />}
                        </Button>
                        <span className="w-8 text-center font-semibold">{item.quantity}</span>
                        <Button
                          size="icon"
                          variant="outline"
                          className="h-7 w-7"
                          onClick={() => addToBasket(item)}
                        >
                          <Plus className="h-3 w-3" />
                        </Button>
                      </div>
                    </div>
                  </div>
                ))}
              </div>

              <div className="border-t pt-4 space-y-4">
                <div className="flex justify-between items-center text-lg font-bold">
                  <span>Total:</span>
                  <span className="text-primary">${total.toFixed(2)}</span>
                </div>

                <Select value={selectedTable} onValueChange={setSelectedTable}>
                  <SelectTrigger>
                    <SelectValue placeholder="Select your table" />
                  </SelectTrigger>
                  <SelectContent>
                    {[1, 2, 3, 4, 5].map(num => (
                      <SelectItem key={num} value={num.toString()}>
                        Table {num}
                      </SelectItem>
                    ))}
                  </SelectContent>
                </Select>

                <Button 
                  className="w-full" 
                  size="lg"
                  onClick={handleConfirm}
                  disabled={!selectedTable || isPlacingOrder}
                >
                  {isPlacingOrder ? 'Placing Order...' : 'Confirm Order'}
                </Button>
              </div>
            </>
          )}
        </div>
      </SheetContent>
    </Sheet>
  );
};
